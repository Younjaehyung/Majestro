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
    nointerpolation uint materialIndex : MaterialIndex;
};


float4 PS_Main(VS_OUT input) : SV_Target
{
   // 임시 RED
    
    // forward 정점 셰이더가 전달한 머티리얼 인덱스를 재사용한다.
    MATERIALINFO materials = Materials[input.materialIndex];
    
    
    
    return float4(materials.ExtValue[0]);

}
