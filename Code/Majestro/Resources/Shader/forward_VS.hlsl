
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
    float4 pos : SV_Position;   //문법상 고정된 이름임.
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;   //T
    float3 viewBinormal : BINORMAL; //B
    
    nointerpolation uint materialIndex : MaterialIndex;
    nointerpolation float4 objectExtra : ObjectExtra;
    nointerpolation float4 objectHighlight : ObjectHighlight;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS Instance = InstanceParams[idx];
    output.materialIndex = Instance.MaterialInfoIndex;
    // 오브젝트 부가 정보는 정점 단계에서 한 번 읽고 픽셀 단계에 전달한다.
    output.objectExtra = Objects[Instance.ObjectIndex].Extra1;
    output.objectHighlight = Objects[Instance.ObjectIndex].Extra2;
    
    
    uint objectIndex = Instance.ObjectIndex;


    matrix WV = mul(Objects[objectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);
    
    if (Instance.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent, input.weight, input.indices, AnimInstance[Instance.LightIndex].ReulstIndex);

    
    output.pos = mul(float4(input.pos, 1.f), WVP);
    output.uv = input.uv;
    
    output.viewPos = mul(float4(input.pos, 1.f), WV).xyz;
    output.viewNormal = normalize(mul(float4(input.normal, 0.f), WV).xyz);
    output.viewTangent = normalize(mul(float4(input.tangent, 0.f), WV).xyz);
    output.viewBinormal = normalize(cross(output.viewTangent, output.viewNormal));
    
    return output;
}

