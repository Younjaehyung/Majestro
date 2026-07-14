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
    float radialMask = 1.0f - saturate(length(centeredUv));
    radialMask = smoothstep(0.0f, 0.8f, radialMask);

    float lifeFadeIn = smoothstep(0.0f, 0.12f, ratio);
    float lifeFadeOut = 1.0f - smoothstep(0.72f, 1.0f, ratio);
    float pulse = 0.72f + 0.28f * sin(PassParams.TotalTime * 6.0f + particle.worldPos.y * 0.08f);
    float energy = saturate(tex.r * 1.4f) * radialMask * lifeFadeIn * lifeFadeOut * pulse;

    float3 coreColor = float3(0.15f, 0.75f, 1.0f);
    float3 edgeColor = float3(0.55f, 0.28f, 1.0f);
    float3 auraColor = lerp(edgeColor, coreColor, saturate(tex.r));

    return float4(auraColor * energy * 1.8f, energy);
}
