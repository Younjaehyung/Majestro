#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;

};

float4 PS_Main(VS_OUT input) : SV_Target
{
    float invW = rcp(max(abs(input.clipPos.w), 1e-5f));
    float depth = saturate(input.clipPos.z * invW);
    return float4(depth, 0.f, 0.f, 0.f);
}
