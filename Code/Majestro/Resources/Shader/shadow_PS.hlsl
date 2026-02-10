#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;

};

float4 PS_Main(VS_OUT input) : SV_Target
{
    float depth = input.clipPos.z / max(input.clipPos.w, 1e-5f);
    return float4(depth, 0.f, 0.f, 1.f);
}
