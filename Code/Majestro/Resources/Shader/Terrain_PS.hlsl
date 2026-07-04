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
    // position RT 제거 — 뷰공간 위치는 PRE_DEPTH(Gbuffer[0]) 역투영으로 재구성
    float4 normal : SV_Target0;
    float4 color : SV_Target1;
};

PS_OUT PS_Main(DS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    float3 accumColor = 0.0f;
    float3 accumNormal = 0.0f;
    float accumW = 0.0f;

    float3 N = normalize(input.viewNormal);
    float3 T = normalize(input.viewTangent);
    float3 B = normalize(input.viewBinormal);
    float3x3 matTBN = float3x3(T, B, N);

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];


    const float MacroTiling = 0.5f; //  0.1(매크로)
    const float DetailTiling = 2.0f; // 2.0(디테일)
    const float NoiseTiling = .5f; // 노이즈 정도

    int slot;

    [unroll]
    for (int i = 0; i < 6; i++)
    {
        if (i == 0)
            slot = PassParams.TerrainSlot1;
        else if (i == 1)
            slot = PassParams.TerrainSlot2;
        else if (i == 2)
            slot = PassParams.TerrainSlot3;
        else if (i == 3)
            slot = PassParams.TerrainSlot4;
        else if (i == 4)
            slot = PassParams.TerrainSlot5;
        else if (i == 5)
            slot = PassParams.TerrainSlot6;

        if (slot < 0)
            continue;

        MATERIALINFO material = Materials[slot];


        if (material.DiffuseMap1Index < 0)
            continue;

        float4 maskTex = TextureMaps[material.DiffuseMap1Index].Sample(g_sam_0, input.fulluv);
        float w = saturate(maskTex.r);

        if (w <= 1e-4f)
            continue;

        // 노이즈 샘플링
        float noiseAlpha = 0.5f;
        if (material.DiffuseMap2Index != 0)
        {
            noiseAlpha = TextureMaps[material.DiffuseMap2Index].Sample(g_sam_0, input.fulluv * NoiseTiling).r;
            noiseAlpha = saturate(noiseAlpha);
        }


        float3 layerColor = 0.0f;
        if (material.DiffuseMap0Index != 0)
        {
            float2 uvMacro = input.uv * MacroTiling;
            float2 uvDetail = input.uv * DetailTiling;

            float3 colA = TextureMaps[material.DiffuseMap0Index].Sample(g_sam_0, uvMacro).rgb;
            float3 colB = TextureMaps[material.DiffuseMap0Index].Sample(g_sam_0, uvDetail).rgb;
            layerColor = lerp(colA, colB, noiseAlpha);
        }


        float3 layerViewNormal = N;
        if (material.NormalMapIndex != 0)
        {
            float2 uvMacro = input.uv * MacroTiling;
            float2 uvDetail = input.uv * DetailTiling; 

            float3 tsnA = TextureMaps[material.NormalMapIndex].Sample(g_sam_0, uvMacro).xyz; 
            float3 tsnB = TextureMaps[material.NormalMapIndex].Sample(g_sam_0, uvDetail).xyz;
            tsnA = tsnA * 2.0f - 1.0f; 
            tsnB = tsnB * 2.0f - 1.0f;

            float3 tsn = normalize(lerp(tsnA, tsnB, noiseAlpha));
            layerViewNormal = normalize(mul(tsn, matTBN)); 
        }

        accumColor += layerColor * w;
        accumNormal += layerViewNormal * w;
        accumW += w;
    }


    if (accumW > 1e-4f)
    {
        accumColor /= accumW;
        accumNormal = normalize(accumNormal);
    }
    else
    {
        accumColor = 0.0f;
        accumNormal = N;
    }

    output.color = float4(accumColor, 1.0f);
    output.normal = float4(accumNormal, 0.0f);
    return output;
}

//#include "params.hlsl"
//#include "utils.hlsl"

//struct DS_OUT
//{
//    float4 pos : SV_Position;
//    float2 uv : TEXCOORD; // 패치 로컬 UV (타일링용)
//    float2 fulluv : TEXCOORD1; // 전체 터레인 UV (마스크용)
//    float3 viewPos : POSITION;
//    float3 viewNormal : NORMAL;
//    float3 viewTangent : TANGENT;
//    float3 viewBinormal : BINORMAL;

//    uint instanceID : InstanceID;
//};

//struct PS_OUT
//{
//    float4 position : SV_Target0;
//    float4 normal : SV_Target1;
//    float4 color : SV_Target2;
//};

//float2 RotateUV(float2 uv, float rad)
//{
//    float s = sin(rad);
//    float c = cos(rad);
//    return float2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);
//}

//float FastHashNoise(float2 uv)
//{
//    return frac(sin(dot(uv, float2(127.1f, 311.7f))) * 43758.5453f);
//}

//float SampleNoiseMask(int noiseTexIndex, float2 noiseUV)
//{
//    if (noiseTexIndex >= 0)
//    {
        
//        float n = TextureMaps[noiseTexIndex].Sample(g_sam_0, noiseUV).r;
//        return smoothstep(0.2f, 0.8f, saturate(n));
//    }
   
//    float n = FastHashNoise(noiseUV);
//    return smoothstep(0.2f, 0.8f, n);
//}

//float3 SampleThreeWayTiling(Texture2D tex, float2 uv, float2 fulluv, int noiseTexIndex, float3 tilingScale, float noiseScale)
//{
//    // 비정수 3-way 스케일 조합 (주기성 완화)
//    float2 uv0 = uv * tilingScale.x;
//    float2 uv1 = RotateUV(uv * tilingScale.y + float2(2.37f, -1.41f), 1.04719755f);
//    float2 uv2 = RotateUV(uv * tilingScale.z + float2(-3.11f, 4.27f), -0.78539816f);

//    // 3-way 가중치 생성용 노이즈(서로 다른 주파수/오프셋)
//    float n0 = SampleNoiseMask(noiseTexIndex, fulluv * noiseScale + float2(0.17f, 0.31f));
//    float n1 = SampleNoiseMask(noiseTexIndex, fulluv * (noiseScale * 1.91f) + float2(4.73f, -2.13f));

//    n0 = FastHashNoise(n0);

//    n1 = FastHashNoise(n1);
    
//    // n0, n1로 3개 weight를 만든 뒤 정규화
//    float w0 = saturate(1.0f - n0);
//    float w1 = saturate(n0 * (1.0f - n1));
//    float w2 = saturate(n0 * n1);

//    float wSum = max(w0 + w1 + w2, 1e-4f);
//    float3 w = float3(w0, w1, w2) / wSum;

//    float3 c0 = tex.Sample(g_sam_0, uv0).rgb;
//    float3 c1 = tex.Sample(g_sam_0, uv1).rgb;
//    float3 c2 = tex.Sample(g_sam_0, uv2).rgb;

//    return c0 * w.x + c1 * w.y + c2 * w.z;
//}


//PS_OUT PS_Main(DS_OUT input)
//{
//    PS_OUT output = (PS_OUT) 0;

   
//    float3 accumColor = 0.0f;
//    float3 accumNormal = 0.0f;
//    float accumW = 0.0f;

   
//    float3 N = normalize(input.viewNormal);
//    float3 T = normalize(input.viewTangent);
//    float3 B = normalize(input.viewBinormal);
//    float3x3 matTBN = float3x3(T, B, N);

//    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
//    RENDERPARAMS instance = InstanceParams[idx];
   

//    int slot;
    
//    [unroll]
//    for (int i = 0; i < 6; i++)
//    {
        
//        if (i == 0)
//            slot = PassParams.TerrainSlot1;
//        else if (i == 1)
//            slot = PassParams.TerrainSlot2;
//        else if (i == 2)
//            slot = PassParams.TerrainSlot3;
//        else if (i == 3)
//            slot = PassParams.TerrainSlot4;
//        else if (i == 4)
//            slot = PassParams.TerrainSlot5;
//        else if (i == 5)
//            slot = PassParams.TerrainSlot6;

        
//        if (slot < 0)
//            continue;

//        MATERIALINFO material = Materials[slot];

       
//        float4 maskTex = TextureMaps[material.DiffuseMap1Index].Sample(g_sam_0, input.fulluv);
        
//        float w = saturate(maskTex.r); // 필요 시 .a 로 변경

        
//        if (w <= 1e-4f)
//            continue;

       
//        // DiffuseMap3Index: 노이즈 텍스처(블루프린트의 Noise Texture Sample 대응)
//        int noiseTexIndex = material.DiffuseMap2Index;

//         // 비정수 스케일 3-way
//        float3 scaleColor = float3(0.93f, 1.71f, 2.89f);
//        float3 scaleNormal = float3(0.91f, 1.67f, 2.77f);

//        float3 layerColor = SampleThreeWayTiling(
//            TextureMaps[material.DiffuseMap0Index],
//            input.uv,
//            input.fulluv,
//            noiseTexIndex,
//            scaleColor,
//            0.29f
//        );

//        float3 tsn = SampleThreeWayTiling(
//            TextureMaps[material.NormalMapIndex],
//            input.uv,
//            input.fulluv,
//            noiseTexIndex,
//            scaleNormal,
//            0.31f
//        );

//        tsn = tsn * 2.0f - 1.0f;
//        float3 layerViewNormal = normalize(mul(tsn, matTBN));

//        accumColor += layerColor * w;
//        accumNormal += layerViewNormal * w;
//        accumW += w;
//    }

//    // 가중치 정규화(합이 1이 되도록). 마스크 총합이 0이면 기본값 사용.
//    if (accumW > 1e-4f)
//    {
//        accumColor /= accumW;
//        accumNormal = normalize(accumNormal);
//    }
//    else
//    {
       
//        accumColor = 0.0f; // 또는 float3(1,1,1)
//        accumNormal = N;
//    }

//    output.color = float4(accumColor, 1.0f);

  
//    output.normal = float4(accumNormal, 0.0f);

   
//    output.position = float4(input.viewPos.xyz, 1.0f);

    
//    return output;
//}
