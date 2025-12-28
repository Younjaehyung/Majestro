
#include "params.hlsl"
#include "utils.hlsl"


struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;

    uint instanceID : InstanceID;
};

// uint Index0 = ObjectIndex;
// uint Index1 = MaterialInfoIndex;

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 color = float4(1.f, 1.f, 1.f, 1.f);
    
    UIInstanceData instance = UIInstances[input.instanceID];
    int materialIndex = instance.MaterialIndex;
    
    MATERIALINFO materials = Materials[materialIndex];
    
    if (materialIndex)
        color = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);

    if (color.a < 0.1f)
        discard;


    
    return color;
}

