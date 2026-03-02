#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;

   
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation uint matIdx : TEXCOORD0; 
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT o = (VS_OUT) 0;

    uint globalInstanceID = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS inst = InstanceParams[globalInstanceID];

    uint objectIndex = inst.ObjectIndex;
    o.matIdx = inst.MaterialInfoIndex;

    matrix WV = mul(Objects[objectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);

    o.pos = mul(float4(input.pos, 1.f), WVP);
    return o;
}
