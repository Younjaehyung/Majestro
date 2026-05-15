#include "params.hlsl"
#include "math.hlsl"
struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation float objectAlpha : TEXCOORD0;
};

void PS_Main(VS_OUT input)
{
    // 카메라 페이드 디더
    if (input.objectAlpha < 0.999f)
    {

        uint2 pix = (uint2) input.pos.xy;
        float threshold = bayer[(pix.y & 3) * 4 + (pix.x & 3)];
        clip(input.objectAlpha - threshold);
    }
}
