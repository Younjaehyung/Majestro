#include "params.hlsl"
#include "utils.hlsl"

struct GS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint id : ID;
};

float4 PS_Main(GS_OUT input) : SV_Target
{
    const uint emitterIndex = GlobalParams.PassScalar0;
    const PARTICLE particle = Particle[input.id];
    const float ratio = saturate(particle.curTime / max(particle.lifeTime, 0.0001f));

    float4 tex = TextureMaps[ParticleShared[emitterIndex].TextureIndex].Sample(g_sam_0, input.uv);
    float2 centeredUv = input.uv * 2.0f - 1.0f;
    float coreMask = 1.0f - saturate(length(centeredUv));
    coreMask = smoothstep(0.0f, 0.85f, coreMask);

    float headFlash = 1.0f - smoothstep(0.0f, 0.28f, ratio);
    float fadeOut = 1.0f - smoothstep(0.35f, 1.0f, ratio);
    float energy = saturate(tex.r * 1.5f) * coreMask * fadeOut;

    float3 hotColor = float3(1.0f, 0.86f, 0.34f);
    float3 coolColor = float3(0.35f, 0.62f, 1.0f);
    float3 trailColor = lerp(coolColor, hotColor, headFlash);

    return float4(trailColor * energy * 1.9f, energy);
}
