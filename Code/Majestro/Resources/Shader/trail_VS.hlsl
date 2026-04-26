#include "params.hlsl"

struct VS_IN
{
    float3 pos      : POSITION;
    float2 uv       : TEXCOORD;
    float3 color    : NORMAL;
    float3 tangent  : TANGENT;
    float4 weight   : BONEWEIGHT;
    float4 indices  : BONEINDICES;
};

struct VS_OUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEXCOORD0;
    float3 color : COLOR0;
    float alpha  : TEXCOORD1;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    // Modified:
    // TrailPass uploads world-space ribbon vertices, so the vertex shader only
    // applies the active camera view/projection matrices from PassParams.
    matrix viewProj = mul(PassParams.MatView, PassParams.MatProjection);
    output.pos = mul(float4(input.pos, 1.f), viewProj);
    output.uv = input.uv;
    output.color = input.color;
    output.alpha = input.tangent.x;
    return output;
}
