#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    uint id : SV_InstanceID;
};

struct VS_OUT
{
    float4 viewPos : POSITION;
    float2 uv : TEXCOORD;
    float id : ID;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0.f;

    const uint emitterIndex = GlobalParams.etc;
    float3 worldPos = mul(float4(input.pos, 1.f), ParticleShared[emitterIndex].MatWorld).xyz;
    worldPos += Particle[input.id].worldPos;

    output.viewPos = mul(float4(worldPos, 1.f), PassParams.MatView);
    output.uv = input.uv;
    output.id = input.id;
    return output;
}
