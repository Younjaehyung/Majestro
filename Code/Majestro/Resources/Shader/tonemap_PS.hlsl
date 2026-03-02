#include "params.hlsl"
#include "utils.hlsl"
struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float3 ReinhardToneMap(float3 hdr)
{
    return hdr / (1.0f + hdr);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float3 hdrColor = Gbuffer[6].Sample(g_sam_0, input.uv).rgb;
    float3 mapped = TonemapACES(hdrColor);
    float3 gammaCorrected = pow(max(mapped, 0.0f), 1.0f / 2.2f);

    return float4(gammaCorrected, 1.0f);
}