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

    float2 uv = input.uv;
    float2 centeredUv = uv * 2.0f - 1.0f;

    float noiseA = TextureMaps[ParticleShared[emitterIndex].TextureIndex].Sample(g_sam_0, uv).r;
    float noiseB = TextureMaps[ParticleShared[emitterIndex].TextureIndex].Sample(g_sam_0, uv * 1.8f + particle.worldDir.xy).r;
    float noiseC = TextureMaps[ParticleShared[emitterIndex].TextureIndex].Sample(g_sam_0, uv * 3.4f + particle.worldDir.yz * 0.71f).r;
    float density = saturate(noiseA * 0.55f + noiseB * 0.35f + noiseC * 0.22f);

    float edgeFade = 1.0f - saturate(length(centeredUv));
    edgeFade = smoothstep(0.02f, 0.72f, edgeFade);

    float lobeA = 1.0f - saturate(length(centeredUv - float2(-0.34f, 0.02f)) / 0.78f);
    float lobeB = 1.0f - saturate(length(centeredUv - float2(0.18f, 0.12f)) / 0.66f);
    float lobeC = 1.0f - saturate(length(centeredUv - float2(0.46f, -0.12f)) / 0.58f);
    float lobeMask = saturate(max(max(lobeA, lobeB), lobeC));
    lobeMask = smoothstep(0.0f, 0.86f, lobeMask);

    float cloudMask = smoothstep(0.34f, 0.84f, density) * lobeMask;
    float lifeFadeIn = smoothstep(0.0f, 0.16f, ratio);
    float lifeFadeOut = 1.0f - smoothstep(0.72f, 1.0f, ratio);

    float alpha = cloudMask * edgeFade * lifeFadeIn * lifeFadeOut * 0.46f;
    float heightTint = saturate((particle.worldPos.y + 1700.0f) / 3400.0f);
    float verticalLight = saturate(uv.y * 0.65f + heightTint * 0.35f);
    float3 shadowColor = float3(0.55f, 0.62f, 0.70f);
    float3 lightColor = float3(0.88f, 0.93f, 0.98f);
    float3 cloudColor = lerp(shadowColor, lightColor, saturate(density * 0.55f + verticalLight * 0.45f));

    return float4(cloudColor, alpha);
}
