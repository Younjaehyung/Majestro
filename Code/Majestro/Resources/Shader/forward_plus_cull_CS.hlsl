#include "params.hlsl"

#define FORWARD_PLUS_TILE_SIZE 16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128

[numthreads(1, 1, 1)]
void CS_Main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint tileCountX = GlobalParams.BaseInstanceID;
    const uint tileCountY = GlobalParams.etc;

    if (dispatchThreadId.x >= tileCountX || dispatchThreadId.y >= tileCountY)
        return;

    const uint tileIndex = dispatchThreadId.y * tileCountX + dispatchThreadId.x;
    const uint tileMinX = dispatchThreadId.x * FORWARD_PLUS_TILE_SIZE;
    const uint tileMinY = dispatchThreadId.y * FORWARD_PLUS_TILE_SIZE;
    const uint tileMaxX = min(tileMinX + FORWARD_PLUS_TILE_SIZE, (uint) PassParams.ScreenSize.x);
    const uint tileMaxY = min(tileMinY + FORWARD_PLUS_TILE_SIZE, (uint) PassParams.ScreenSize.y);

    const uint startOffset = tileIndex * FORWARD_PLUS_MAX_LIGHTS_PER_TILE;
    uint count = 0;

    [loop]
    for (uint lightIndex = 0; lightIndex < (uint) PassParams.LightsCount; ++lightIndex)
    {
        const LIGHTINFO light = Lights[lightIndex];

        bool intersects = false;
        if (light.lightType == 0)
        {
            intersects = true;
        }
        else
        {
            const float3 viewLightPos = mul(float4(light.position.xyz, 1.f), PassParams.MatView).xyz;
            const float4 clipCenter = mul(float4(viewLightPos, 1.f), PassParams.MatProjection);

            if (clipCenter.w > 1e-5f)
            {
                const float2 ndcCenter = clipCenter.xy / clipCenter.w;
                const float2 pixelCenter = float2((ndcCenter.x * 0.5f + 0.5f) * PassParams.ScreenSize.x,
                                                  (-ndcCenter.y * 0.5f + 0.5f) * PassParams.ScreenSize.y);

                const float3 viewOffset = viewLightPos + float3(light.range, 0.f, 0.f);
                const float4 clipOffset = mul(float4(viewOffset, 1.f), PassParams.MatProjection);
                const float2 ndcOffset = clipOffset.xy / max(clipOffset.w, 1e-5f);
                float radiusPixels = abs((ndcOffset.x - ndcCenter.x) * 0.5f * PassParams.ScreenSize.x);

                radiusPixels = max(radiusPixels, 2.0f);

                const float tileMinXF = (float) tileMinX;
                const float tileMinYF = (float) tileMinY;
                const float tileMaxXF = (float) tileMaxX;
                const float tileMaxYF = (float) tileMaxY;

                intersects = !(pixelCenter.x + radiusPixels < tileMinXF ||
                               pixelCenter.x - radiusPixels > tileMaxXF ||
                               pixelCenter.y + radiusPixels < tileMinYF ||
                               pixelCenter.y - radiusPixels > tileMaxYF);
            }
        }

        if (intersects && count < FORWARD_PLUS_MAX_LIGHTS_PER_TILE)
        {
            RWForwardPlusLightIndices[startOffset + count] = lightIndex;
            count++;
        }
    }

    RWForwardPlusTileMeta[tileIndex] = uint2(startOffset, count);
}