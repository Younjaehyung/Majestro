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
    float edgeFade = 1.0f - saturate(length(centeredUv));
    edgeFade = smoothstep(0.0f, 0.65f, edgeFade);

    float lifeFadeIn = smoothstep(0.0f, 0.18f, ratio);
    float lifeFadeOut = 1.0f - smoothstep(0.62f, 1.0f, ratio);
    float density = saturate(tex.r * 1.25f);
    float alpha = density * edgeFade * lifeFadeIn * lifeFadeOut * 0.48f;

    float3 smokeColor = lerp(float3(0.20f, 0.20f, 0.20f), float3(0.58f, 0.58f, 0.58f), density);
    return float4(smokeColor, alpha);
}
