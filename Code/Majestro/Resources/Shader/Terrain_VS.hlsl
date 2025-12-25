#include "params.hlsl"
#include "utils.hlsl"


// --------------
// Vertex Shader
// --------------

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 uv : TEXCOORD;

    
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    
    uint instanceID : InstanceID;
};

VS_OUT VS_Main(VS_INPUT input)
{
    VS_OUT output = (VS_OUT)0;
    output.instanceID = input.instanceID;
    output.pos = input.Position;
    output.uv = input.uv;
    
    
    return output;
}
