
#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    
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
    
    uint instanceID : InstanceID;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    output.instanceID = input.instanceID;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS Instance = InstanceParams[idx];
    
    
    uint objectIndex = Instance.ObjectIndex;


    matrix WV = mul(Objects[objectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);
    
    output.pos = mul(float4(input.pos, 1.f), WVP);
    output.uv = input.uv;
    
    output.viewPos = mul(float4(input.pos, 1.f), WV).xyz;
    output.viewNormal = normalize(mul(float4(input.normal, 0.f), WV).xyz);
    output.viewTangent = normalize(mul(float4(input.tangent, 0.f), WV).xyz);
    output.viewBinormal = normalize(cross(output.viewTangent, output.viewNormal));
    
    return output;
}

