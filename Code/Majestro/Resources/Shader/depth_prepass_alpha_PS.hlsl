#include "params.hlsl"

// VS는 forward_alpha_VS.hlsl 공용
struct VS_OUT
{
    float4 pos          : SV_Position;
    float2 uv           : TEXCOORD;
    float3 viewPos      : POSITION;
    float3 viewNormal   : NORMAL;
    float3 viewTangent  : TANGENT;
    float3 viewBinormal : BINORMAL;
    uint   instanceID   : InstanceID;
};

// 알파값전용 Depth Pre-Pass.
void PS_Main(VS_OUT input)
{
    const uint idx          = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS inst = InstanceParams[idx];
    const MATERIALINFO mtl  = Materials[inst.MaterialInfoIndex];

    float alpha = mtl.Diffuse.a;
    if (mtl.DiffuseMap0Index >= 0)
        alpha *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv).a;

    if (alpha < 0.001f)
        clip(-1);
}
