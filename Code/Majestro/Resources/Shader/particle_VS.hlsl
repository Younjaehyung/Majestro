
#include "params.hlsl"
#include "utils.hlsl"



struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    uint id : SV_InstanceID;    //instance ID
};

struct VS_OUT
{
    float4 viewPos : POSITION;
    float2 uv : TEXCOORD;
    float id : ID;
};

// VS_MAIN
// g_float_0    : Start Scale
// g_float_1    : End Scale
// g_tex_0      : Particle Texture

// uint Index0 = ParticleIndex;

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0.f;

    float3 worldPos = mul(float4(input.pos, 1.f), Objects[GlobalParams.BaseInstanceID].MatWorld).xyz;
    worldPos += RWParticle[input.id].worldPos;

    output.viewPos = mul(float4(worldPos, 1.f), PassParams.MatView);
    output.uv = input.uv;
    output.id = input.id;

    return output;
}
