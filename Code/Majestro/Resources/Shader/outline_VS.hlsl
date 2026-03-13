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
    float4 pos                      : SV_Position;
    nointerpolation uint matIdx     : TEXCOORD0;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT o = (VS_OUT) 0;

    uint globalInstanceID = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS inst = InstanceParams[globalInstanceID];
    o.matIdx = inst.MaterialInfoIndex;


    if (inst.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent,
                 input.weight, input.indices,
                 AnimInstance[inst.LightIndex].ReulstIndex);

    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    
    // 아웃라인 두께
    MATERIALINFO mat = Materials[inst.MaterialInfoIndex];
    float outlineWidth = data.ExtValue[0].x;

    matrix WV  = mul(Objects[inst.ObjectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);

    // View Space 법선 계산
    float3 viewNormal = normalize(mul((float3x3) WV, input.normal));

    // Clip Space 팽창 (w 보정으로 원근에 무관하게 균일한 두께 유지)
    float4 clipPos = mul(float4(input.pos, 1.0f), WVP);
    clipPos.xy += viewNormal.xy * outlineWidth * clipPos.w;

    o.pos = clipPos;
    return o;
}
