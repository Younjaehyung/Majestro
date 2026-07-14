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

struct GS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint id : ID;
};

GS_OUT VS_Main(VS_IN input)
{
    GS_OUT output = (GS_OUT)0.0f;

    const uint id = input.id;
    const uint emitterIndex = GlobalParams.PassScalar0;

    float ratio = saturate(Particle[id].curTime / max(Particle[id].lifeTime, 0.0001f));
    float scale = lerp(ParticleShared[emitterIndex].StartScale, ParticleShared[emitterIndex].EndScale, ratio);
    scale *= Particle[id].alive != 0 ? 1.0f : 0.0f;

    float3 worldPos = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ParticleShared[emitterIndex].MatWorld).xyz;
    worldPos += Particle[id].worldPos;

    float4 viewPos = mul(float4(worldPos, 1.0f), PassParams.MatView);

    float2 corner = input.pos.xy;
    viewPos.xy += corner * scale;

    output.position = mul(viewPos, PassParams.MatProjection);
    output.uv = input.uv;
    output.id = id;
    return output;
}
