#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos     : POSITION;
    float2 uv      : TEXCOORD;
    float3 normal  : NORMAL;
    float3 tangent : TANGENT;
    float4 weight  : BONEWEIGHT;
    float4 indices : BONEINDICES;

    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos        : SV_Position;
    float2 uv         : TEXCOORD;
    uint   instanceID : InstanceID;
};


VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];

    if (instance.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent,
                 input.weight, input.indices,
                 AnimInstance[instance.LightIndex].ReulstIndex);

    matrix matWorld = Objects[instance.ObjectIndex].MatWorld;
    matrix VP = mul(PassParams.MatView, PassParams.MatProjection);

  
    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];
    float3 worldPos = mul(float4(input.pos, 1.f), matWorld).xyz;
    worldPos += CalcWindOffset(worldPos, input.pos.y, mtl.ExtValue[0]);

    output.pos        = mul(float4(worldPos, 1.f), VP);
    
    // ForwardAlpha(LESS_EQUAL)와의 FP 오차로 인한 depth test 실패 방지용 bias
    output.pos.z     += output.pos.w * 0.0002f;
    output.uv         = input.uv;
    output.instanceID = input.instanceID;

    return output;
}
