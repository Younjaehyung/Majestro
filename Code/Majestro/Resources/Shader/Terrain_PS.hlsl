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

   
    float3 accumColor = 0.0f;
    float3 accumNormal = 0.0f;
    float accumW = 0.0f;

   
    float3 N = normalize(input.viewNormal);
    float3 T = normalize(input.viewTangent);
    float3 B = normalize(input.viewBinormal);
    float3x3 matTBN = float3x3(T, B, N);

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
   

    int slot;
    
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

       
        float4 maskTex = TextureMaps[material.DiffuseMap1Index].Sample(g_sam_0, input.fulluv);
        
        float w = saturate(maskTex.r); // 필요 시 .a 로 변경

        
        if (w <= 1e-4f)
            continue;

       
        float3 layerColor = TextureMaps[material.DiffuseMap0Index].Sample(g_sam_0, input.uv).rgb;

        
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
       
        accumColor = 0.0f; // 또는 float3(1,1,1)
        accumNormal = N;
    }

    output.color = float4(accumColor, 1.0f);

  
    output.normal = float4(accumNormal, 0.0f);

   
    output.position = float4(input.viewPos.xyz, 1.0f);

    
    return output;
}
