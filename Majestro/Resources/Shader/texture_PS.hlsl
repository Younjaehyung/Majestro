
#include "params.hlsl"
#include "utils.hlsl"


struct VS_TEX_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 PS_Tex(VS_TEX_OUT input) : SV_Target
{
    float4 color = float4(1.f, 1.f, 1.f, 1.f);
    int materialIndex = Objects[GlobalParams.ObjectIndex].MaterialInfoIndex;
    
    MATERIALINFO materials = Materials[materialIndex];
    
    if (materialIndex)
        color = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);

    return color;
}
