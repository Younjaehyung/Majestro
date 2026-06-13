#include "params.hlsl"
#include "math.hlsl"
#include "utils.hlsl"
struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation float objectAlpha : TEXCOORD0;
    float2 uv : TEXCOORD1;
    nointerpolation float dissolve : TEXCOORD2;
    nointerpolation int noiseTexIdx : TEXCOORD3;
};

void PS_Main(VS_OUT input)
{
    ApplyIGNDitherFade(input.pos.xy, input.objectAlpha);

    // 디졸브시
    float dissolve = saturate(input.dissolve);
    if (dissolve > 0.0f)
    {
        float dnoise = SampleDissolveNoise(input.noiseTexIdx, input.uv);
        if (dnoise < dissolve)
            discard;
    }
}
