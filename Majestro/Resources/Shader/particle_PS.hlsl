
#include "params.hlsl"
#include "utils.hlsl"


struct VS_OUT
{
    float4 viewPos : POSITION;
    float2 uv : TEXCOORD;
    float id : ID;
};

struct GS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint id : SV_InstanceID;
};


float4 PS_Main(GS_OUT input) : SV_Target
{
    return TextureMaps[ParticleShared[GlobalParams.ParticleIndex].TextureIndex].Sample(g_sam_0, input.uv);
}



