#include "params.hlsl"
#include "math.hlsl"
#include "utils.hlsl"
struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation float objectAlpha : TEXCOORD0;
};

void PS_Main(VS_OUT input)
{
    ApplyIGNDitherFade(input.pos.xy, input.objectAlpha);

}
