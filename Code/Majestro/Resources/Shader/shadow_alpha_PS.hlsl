#include "params.hlsl"

struct VS_OUT
{
    float4 pos        : SV_Position;
    float4 clipPos    : POSITION;
    float2 uv         : TEXCOORD;
    uint   instanceID : InstanceID;
};

float PS_Main(VS_OUT input) : SV_Depth
{
    const uint idx          = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS inst = InstanceParams[idx];
    const MATERIALINFO mtl  = Materials[inst.MaterialInfoIndex];

    float alpha = mtl.Diffuse.a;
    if (mtl.DiffuseMap0Index >= 0)
        alpha *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv).a;

    if (alpha < 0.001f)
        clip(-1);

    float invW = rcp(max(abs(input.clipPos.w), 1e-5f));
    return saturate(input.clipPos.z * invW);
}
