
#include "params.hlsl"


struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 color = TextureMaps[PassParams.SkyBoxIndex].Sample(g_sam_0, input.uv);
    return color;
}
