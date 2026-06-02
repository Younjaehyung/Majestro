#include "params.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 color : NORMAL;
    float3 tangent : TANGENT;
    float4 weight : BONEWEIGHT;
    float4 indices : BONEINDICES;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float3 color : COLOR0;
    float alpha : TEXCOORD1;
    float ageRate : TEXCOORD2;
    float intensity : TEXCOORD3;
    float texIndex : TEXCOORD4;
    float layerIndex : TEXCOORD5;
    float style : TEXCOORD6;
    float cutStrength : TEXCOORD7;
    float4 subColorAndLine : TEXCOORD8;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    matrix viewProj = mul(PassParams.MatView, PassParams.MatProjection);
    output.pos = mul(float4(input.pos, 1.0f), viewProj);
    output.uv = input.uv;
    output.color = input.color;
    output.alpha = input.tangent.x;
    output.ageRate = input.tangent.y;
    output.intensity = input.tangent.z;
    output.texIndex = input.weight.x;
    output.layerIndex = input.weight.y;
    output.style = input.weight.z;
    output.cutStrength = input.weight.w;
    output.subColorAndLine = input.indices;
    return output;
}
