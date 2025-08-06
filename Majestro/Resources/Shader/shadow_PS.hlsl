#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    return float4(input.clipPos.z / input.clipPos.w, 0.f, 0.f, 0.f);
}
