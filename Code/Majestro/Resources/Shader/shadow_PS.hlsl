#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;

};

float PS_Main(VS_OUT input) : SV_Depth
{
    float invW = rcp(max(abs(input.clipPos.w), 1e-5f));
    
    return saturate(input.clipPos.z * invW);
}
