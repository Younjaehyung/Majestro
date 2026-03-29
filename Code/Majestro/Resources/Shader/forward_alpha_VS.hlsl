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
    float4 pos          : SV_Position;
    float2 uv           : TEXCOORD;
    float3 viewPos      : POSITION;
    float3 viewNormal   : NORMAL;
    float3 viewTangent  : TANGENT;
    float3 viewBinormal : BINORMAL;
    uint   instanceID   : InstanceID;
};

// 바람 파라미터: MATERIALINFO.ExtValue[0]
//   x = 흔들림 세기  (0 = 비활성, 권장 0.1~0.3)
//   y = 주파수       (권장 0.8~2.0)
//   z = 바람 방향 X
//   w = 바람 방향 Z

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;
    output.instanceID = input.instanceID;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];

    if (instance.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent,
                 input.weight, input.indices,
                 AnimInstance[instance.LightIndex].ReulstIndex);

    matrix matWorld = Objects[instance.ObjectIndex].MatWorld;
    
    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];
    float3 windOfs = CalcWindOffset(
        mul(float4(input.pos, 1.f), matWorld).xyz,
        input.pos.y,
        mtl.ExtValue[0]);
    
    float3 worldPos = mul(float4(input.pos, 1.f), matWorld).xyz + windOfs;

    matrix VP = mul(PassParams.MatView, PassParams.MatProjection);
    matrix WV = mul(matWorld, PassParams.MatView);

    output.pos          = mul(float4(worldPos, 1.f), VP);
    output.uv           = input.uv;
    output.viewPos      = mul(float4(worldPos, 1.f), PassParams.MatView).xyz;
    
    output.viewNormal   = normalize(mul(float4(input.normal,  0.f), WV).xyz);
    output.viewTangent  = normalize(mul(float4(input.tangent, 0.f), WV).xyz);
    output.viewBinormal = normalize(cross(output.viewTangent, output.viewNormal));

    return output;
}
