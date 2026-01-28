#include "params.hlsl"
#include "utils.hlsl"

struct DS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD; // 패치 로컬 UV (타일링용)
    float2 fulluv : TEXCOORD1; // 전체 터레인 UV (마스크용)
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;
    float3 viewBinormal : BINORMAL;

    uint instanceID : InstanceID;
};

struct PS_OUT
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
};

PS_OUT PS_Main(DS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    // [수정] 누적 버퍼를 명확히 0으로 시작
    float3 accumColor = 0.0f;
    float3 accumNormal = 0.0f;
    float accumW = 0.0f;

    // [수정] TBN은 공통으로 한 번만 구성 (view space 기준)
    float3 N = normalize(input.viewNormal);
    float3 T = normalize(input.viewTangent);
    float3 B = normalize(input.viewBinormal);
    float3x3 matTBN = float3x3(T, B, N);

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    // WV/WVP는 여기 PS에서 사용 안 하므로 제거 가능 (지금 코드에서도 미사용)

    int slot;
    // 6개 레이어 블렌딩
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        
        if(i==0) slot = PassParams.TerrainSlot1;
        else if(i==1) slot = PassParams.TerrainSlot2;
        else if(i==2) slot = PassParams.TerrainSlot3;
        else if(i==3) slot = PassParams.TerrainSlot4;
        else if(i==4) slot = PassParams.TerrainSlot5;
        else if (i == 5) slot = PassParams.TerrainSlot6;

        
        if (slot < 0)
            continue;

        MATERIALINFO material = Materials[slot];

        // [수정] 마스크는 "전체 터레인 UV"로 샘플링
        float4 maskTex = TextureMaps[material.DiffuseMap1Index].Sample(g_sam_0, input.fulluv);

        // [수정] 마스크를 '가중치'로 사용 (흑백 마스크라면 보통 r)
        float w = saturate(maskTex.r); // 필요 시 .a 로 변경

        // [수정] 0에 가까우면 샘플 비용 줄이기
        if (w <= 1e-4f)
            continue;

        // [수정] 레이어 컬러는 '패치 로컬 UV'로 샘플링(타일링 목적)
        float3 layerColor = TextureMaps[material.DiffuseMap0Index].Sample(g_sam_0, input.uv).rgb;

        // [수정] 노멀맵도 해당 레이어 weight로 블렌딩
        float3 tsn = TextureMaps[material.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
        tsn = tsn * 2.0f - 1.0f; // tangent space normal [-1,1]
        float3 layerViewNormal = normalize(mul(tsn, matTBN)); // TS -> View

        accumColor += layerColor * w;
        accumNormal += layerViewNormal * w;
        accumW += w;
    }

    // [수정] 가중치 정규화(합이 1이 되도록). 마스크 총합이 0이면 기본값 사용.
    if (accumW > 1e-4f)
    {
        accumColor /= accumW;
        accumNormal = normalize(accumNormal);
    }
    else
    {
        // 마스크가 전부 0인 곳: 기본 노멀/컬러(원하는 정책으로 변경 가능)
        accumColor = 0.0f; // 또는 float3(1,1,1)
        accumNormal = N;
    }

    output.color = float4(accumColor, 1.0f);

    // [수정] GBuffer 노멀은 보통 [0,1]로 패킹해서 저장함. (너 파이프라인에 맞춰 선택)
    // 1) 이미 후처리/라이팅에서 -1~1 그대로 쓴다면 그대로 저장:
    output.normal = float4(accumNormal, 0.0f);

    // 2) 만약 GBuffer에 [0,1] 패킹 저장이 필요하면 아래로 교체:
    // output.normal = float4(accumNormal * 0.5f + 0.5f, 0.0f);

    // [수정] position w는 보통 1.0f가 안전 (너의 GBuffer 정책에 맞춰)
    output.position = float4(input.viewPos.xyz, 1.0f);

    return output;
}
