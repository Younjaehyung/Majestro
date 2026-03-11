
#include "params.hlsl"
#include "utils.hlsl"


struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


float4 PS_Main(VS_OUT input) : SV_Target0
{
   // return Gbuffer[10].Sample(g_sam_0, input.uv);
    return Gbuffer[9].Sample(g_sam_0, input.uv);
}
