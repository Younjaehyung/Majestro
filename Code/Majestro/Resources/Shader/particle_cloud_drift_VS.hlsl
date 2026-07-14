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
    const PARTICLE particle = Particle[id];

    float ratio = saturate(particle.curTime / max(particle.lifeTime, 0.0001f));
    float baseScale = lerp(ParticleShared[emitterIndex].StartScale, ParticleShared[emitterIndex].EndScale, ratio);
    baseScale *= particle.alive != 0 ? 1.0f : 0.0f;

    // Plaza cloud size variation.
    // The drift compute shader stores per particle random values in worldDir,
    // so cloud chunks do not share one uniform billboard size.
    float widthScale = lerp(0.58f, 1.85f, particle.worldDir.x);
    float heightScale = lerp(0.42f, 1.35f, particle.worldDir.y);
    float growthOffset = lerp(-0.18f, 0.24f, particle.worldDir.z);
    float growthScale = 1.0f + growthOffset * smoothstep(0.15f, 0.85f, ratio);

    float2 cloudScale = float2(baseScale * widthScale, baseScale * heightScale) * growthScale;

    float3 worldPos = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), ParticleShared[emitterIndex].MatWorld).xyz;
    worldPos += particle.worldPos;

    float4 viewPos = mul(float4(worldPos, 1.0f), PassParams.MatView);

    // Cloud billboards rotate per particle to avoid repeated flat rectangles.
    // The rotation is visual only and does not affect the drift simulation.
    float rotation = particle.worldDir.z * 6.2831853f + sin(PassParams.TotalTime * 0.03f + (float)id) * 0.08f;
    float c = cos(rotation);
    float s = sin(rotation);
    float2 corner = float2(
        input.pos.x * c - input.pos.y * s,
        input.pos.x * s + input.pos.y * c);
    viewPos.xy += corner * cloudScale;

    output.position = mul(viewPos, PassParams.MatProjection);
    output.uv = input.uv;
    output.id = id;
    return output;
}
