#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation uint matIdx : TEXCOORD0;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    return Materials[input.matIdx].Diffuse;
    //return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
