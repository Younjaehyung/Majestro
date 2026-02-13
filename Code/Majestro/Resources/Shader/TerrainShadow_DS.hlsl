#include "params.hlsl"
#include "utils.hlsl"

struct PatchTess
{
    float edgeTess[3] : SV_TessFactor;
    float insideTess : SV_InsideTessFactor;
    uint instanceID : InstanceID;
};

struct HS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    uint instanceID : InstanceID;
};

struct DS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;
};

[domain("tri")]
DS_OUT DS_Main(const OutputPatch<HS_OUT, 3> input, float3 location : SV_DomainLocation, PatchTess patch)
{
    DS_OUT output = (DS_OUT) 0.f;

    uint idx = GlobalParams.BaseInstanceID + input[0].instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    MATERIALINFO material = Materials[instance.MaterialInfoIndex];

    uint cascadeIndex = min(GlobalParams.casdcae, 3);
    matrix shadowVP = PassParams.CascadeShadowVP[cascadeIndex];
    matrix WVP = mul(Objects[instance.ObjectIndex].MatWorld, shadowVP);

    float3 localPos = input[0].pos * location[0] + input[1].pos * location[1] + input[2].pos * location[2];
    float2 uv = input[0].uv * location[0] + input[1].uv * location[1] + input[2].uv * location[2];

    float2 fullUV = float2(uv.x / (float) PassParams.TileCountX, uv.y / (float) PassParams.TileCountZ);
    float height = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, fullUV, 0).x;
    localPos.y = (height - 0.5f) * 512.0f;

    output.pos = mul(float4(localPos, 1.f), WVP);
    output.clipPos = output.pos;
    return output;
}
