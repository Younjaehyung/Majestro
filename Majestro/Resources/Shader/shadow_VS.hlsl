
#include "params.hlsl"


struct VS_IN
{
    float3 pos : POSITION;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0.f;
    int index = GlobalParams.ObjectIndex;
   
    output.pos = mul(float4(input.pos, 1.f), mul(Objects[index].MatWorld, mul(PassParams.MatView, PassParams.MatProjection)));
    output.clipPos = output.pos;

    return output;
}
