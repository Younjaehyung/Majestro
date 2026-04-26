#include "params.hlsl"

struct VS_OUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEXCOORD0;
    float3 color : COLOR0;
    float alpha  : TEXCOORD1;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    // Modified:
    // Fade the ribbon at both side edges and keep the newest segment slightly
    // brighter so the swing direction remains readable on screen.
    float edge = 1.f - abs(input.uv.x * 2.f - 1.f);
    edge = saturate(edge);
    edge *= edge;

    float headBoost = lerp(0.85f, 1.2f, saturate(input.uv.y));
    float alpha = saturate(input.alpha * edge);
    float3 color = input.color * headBoost;
    return float4(color, alpha);
}
