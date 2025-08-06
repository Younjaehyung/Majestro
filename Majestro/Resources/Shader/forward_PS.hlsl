
#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;   //문법상 고정된 이름임.
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;   //T
    float3 viewBinormal : BINORMAL; //B
};


float4 PS_Main(VS_OUT input) : SV_Target
{

    uint objectIndex = GlobalParams.ObjectIndex;
    int materialIndex = Objects[GlobalParams.ObjectIndex].MaterialInfoIndex;
    MATERIALINFO materials = Materials[materialIndex];

    float4 color = materials.Diffuse;
    
    
       // 다중 텍스처링
    if (materials.DiffuseMap0Index >= 0)
    {
        // 텍스처 배열에서 인덱스에 해당하는 텍스처를 샘플링
        float4 texColor0 = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);
        color *= texColor0;
    }
    
    if (materials.DiffuseMap1Index >= 0)
    {
        // 샘플링 후 블렌딩 (선형 보간)
        float4 texColor1 = TextureMaps[materials.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        // alpha값을 사용하여 블렌딩
        color = lerp(color, texColor1, texColor1.a);
    }
    
    if (materials.DiffuseMap2Index >= 0)
    {

        float4 texColor1 = TextureMaps[materials.DiffuseMap2Index].Sample(g_sam_0, input.uv);

        color = lerp(color, texColor1, texColor1.a);
    }
    
    if (materials.DiffuseMap3Index >= 0)
    {

        float4 texColor1 = TextureMaps[materials.DiffuseMap3Index].Sample(g_sam_0, input.uv);

        color = lerp(color, texColor1, texColor1.a);
    }
    
    
    float3 viewNormal = input.viewNormal;
    if (materials.NormalMapIndex >= 0)
    {
        // [0,255] 범위에서 [0,1]로 변환
        float3 tangentSpaceNormal = TextureMaps[materials.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
        // [0,1] 범위에서 [-1,1]로 변환
        tangentSpaceNormal = (tangentSpaceNormal - 0.5f) * 2.f;
        float3x3 matTBN = { input.viewTangent, input.viewBinormal, input.viewNormal };
        viewNormal = normalize(mul(tangentSpaceNormal, matTBN));
    }

    LightColor totalColor = (LightColor)0.f;
    
    for (int i = 0; i < PassParams.LightsCount; ++i)
    {
        LightColor color = CalculateLightColor(i, viewNormal, input.viewPos);
         totalColor.diffuse += color.diffuse;
         totalColor.ambient += color.ambient;
         totalColor.specular += color.specular;
    }

    color.xyz = (totalColor.diffuse.xyz * color.xyz)
        + totalColor.ambient.xyz * color.xyz
        + totalColor.specular.xyz;

     return color;
}


