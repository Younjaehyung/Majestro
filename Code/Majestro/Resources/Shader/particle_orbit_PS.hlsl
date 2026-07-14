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
    coreMask = smoothstep(0.0f, 0.9f, coreMask);

    float lifeFadeIn = smoothstep(0.0f, 0.10f, ratio);
    float lifeFadeOut = 1.0f - smoothstep(0.78f, 1.0f, ratio);
    float twinkle = 0.65f + 0.35f * sin(PassParams.TotalTime * 9.0f + particle.worldDir.z * 12.0f);
    float energy = saturate(tex.r * 1.6f) * coreMask * lifeFadeIn * lifeFadeOut * twinkle;

    float heightTint = saturate((particle.worldPos.y - 24.0f) / 80.0f);
    float3 lowColor = float3(1.0f, 0.70f, 0.22f);
    float3 highColor = float3(0.28f, 0.85f, 1.0f);
    float3 sparkColor = lerp(lowColor, highColor, heightTint);

    return float4(sparkColor * energy * 1.7f, energy);
}
