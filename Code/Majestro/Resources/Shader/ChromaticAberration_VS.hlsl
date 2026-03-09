#include "params.hlsl"
#include "utils.hlsl"

// PostProcess_ChromaticAberration.hlsl

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    
    VS_OUT output;

    output.pos = float4(input.pos * 2.f, 1.f);
    output.uv = input.uv;

    return output;
}

