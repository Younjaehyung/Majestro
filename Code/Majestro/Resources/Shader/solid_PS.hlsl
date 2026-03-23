#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;
    float3 viewBinormal : BINORMAL;
    uint instanceID : InstanceID;
};


float4 PS_Main(VS_OUT input) : SV_Target
{
   // 임시 RED
    
    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    uint materialIndex = instance.MaterialInfoIndex;
    MATERIALINFO materials = Materials[materialIndex];
    
    
    
    return float4(materials.ExtValue[0]);

}
