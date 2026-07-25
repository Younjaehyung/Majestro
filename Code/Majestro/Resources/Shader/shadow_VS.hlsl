
#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
   
   
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weight : BONEWEIGHT;
    float4 indices : BONEINDICES;
    
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float4 clipPos : POSITION;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0.f;
    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    int index = instance.ObjectIndex;

  
    uint cascadeIndex = min(GlobalParams.PassScalar1, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT - 1);
    matrix shadowVP = PassParams.CascadeShadowVP[cascadeIndex];
    
    
    if (instance.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent, input.weight, input.indices, AnimInstance[instance.LightIndex].ReulstIndex);

    output.pos = mul(float4(input.pos, 1.f), mul(Objects[index].MatWorld, shadowVP));
    output.clipPos = output.pos;
    

    return output;
}
