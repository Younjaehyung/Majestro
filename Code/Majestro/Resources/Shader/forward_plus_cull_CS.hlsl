#include "params.hlsl"

#define FORWARD_PLUS_TILE_SIZE           16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128
#define THREADS_PER_GROUP                (FORWARD_PLUS_TILE_SIZE * FORWARD_PLUS_TILE_SIZE) // 256


// 타일 하나의 컬링 결과를 스레드 간 공유
groupshared uint gs_LightCount;
groupshared uint gs_LightIndices[FORWARD_PLUS_MAX_LIGHTS_PER_TILE];


// [numthreads(16,16,1)]  타일당 256 스레드 병렬 실행
// Dispatch(tileCountX, tileCountY, 1)
[numthreads(FORWARD_PLUS_TILE_SIZE, FORWARD_PLUS_TILE_SIZE, 1)]
void CS_Main(
    uint3 groupId       : SV_GroupID,      // 타일 좌표 (x=열, y=행)
    uint3 groupThreadId : SV_GroupThreadID,
    uint  groupIndex    : SV_GroupIndex)   // 그룹 내 선형 인덱스 (0~255)
{
    // 타일 경계 계산 
    const uint tileCountX = GlobalParams.BaseInstanceID;

    const uint tileIndex = groupId.y * tileCountX + groupId.x;
    const uint tileMinX  = groupId.x * FORWARD_PLUS_TILE_SIZE;
    const uint tileMinY  = groupId.y * FORWARD_PLUS_TILE_SIZE;
    const uint tileMaxX  = min(tileMinX + FORWARD_PLUS_TILE_SIZE, (uint) PassParams.ScreenSize.x);
    const uint tileMaxY  = min(tileMinY + FORWARD_PLUS_TILE_SIZE, (uint) PassParams.ScreenSize.y);

    const uint startOffset = tileIndex * FORWARD_PLUS_MAX_LIGHTS_PER_TILE;
    const uint totalLights = (uint) PassParams.LightsCount;

    // 공유 카운터 초기화 (스레드 0만 실행) 
    if (groupIndex == 0)
        gs_LightCount = 0;

    GroupMemoryBarrierWithGroupSync();

    // 병렬 컬링 루프 
    [loop]
    for (uint lightBase = 0; lightBase < totalLights; lightBase += THREADS_PER_GROUP)
    {
        const uint lightIndex = lightBase + groupIndex;
        if (lightIndex >= totalLights)
            break;

        const LIGHTINFO light = Lights[lightIndex];

        bool intersects = false;

        if (light.lightType == 0)
        {
            // 방향광은 모든 타일에 포함
            intersects = true;
        }
        else
        {
            const float3 viewLightPos = mul(float4(light.position.xyz, 1.f), PassParams.MatView).xyz;
            const float4 clipCenter   = mul(float4(viewLightPos, 1.f), PassParams.MatProjection);

            if (clipCenter.w > 1e-5f)
            {
                const float2 ndcCenter   = clipCenter.xy / clipCenter.w;
                const float2 pixelCenter = float2(
                    ( ndcCenter.x * 0.5f + 0.5f) * PassParams.ScreenSize.x,
                    (-ndcCenter.y * 0.5f + 0.5f) * PassParams.ScreenSize.y);

                const float3 viewOffset  = viewLightPos + float3(light.range, 0.f, 0.f);
                const float4 clipOffset  = mul(float4(viewOffset, 1.f), PassParams.MatProjection);
                const float2 ndcOffset   = clipOffset.xy / max(clipOffset.w, 1e-5f);
                float radiusPixels = abs((ndcOffset.x - ndcCenter.x) * 0.5f * PassParams.ScreenSize.x);
                radiusPixels = max(radiusPixels, 2.0f);

                intersects = !(pixelCenter.x + radiusPixels < (float) tileMinX ||
                               pixelCenter.x - radiusPixels > (float) tileMaxX ||
                               pixelCenter.y + radiusPixels < (float) tileMinY ||
                               pixelCenter.y - radiusPixels > (float) tileMaxY);
            }
        }

        // 교차하는 라이트를 공유 메모리에 원자적으로 추가
        if (intersects)
        {
            uint slot;
            InterlockedAdd(gs_LightCount, 1u, slot);
            if (slot < FORWARD_PLUS_MAX_LIGHTS_PER_TILE)
                gs_LightIndices[slot] = lightIndex;
        }
    }

    // 모든 스레드의 컬링  대기
    GroupMemoryBarrierWithGroupSync();

    
    const uint finalCount = min(gs_LightCount, FORWARD_PLUS_MAX_LIGHTS_PER_TILE);

    // 타일 메타 (스레드 0만 기록)
    if (groupIndex == 0)
        RWForwardPlusTileMeta[tileIndex] = uint2(startOffset, finalCount);

  
    [loop]
    for (uint i = groupIndex; i < finalCount; i += THREADS_PER_GROUP)
        RWForwardPlusLightIndices[startOffset + i] = gs_LightIndices[i];
}