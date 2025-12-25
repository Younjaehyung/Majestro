#include "params.hlsl"
#include "utils.hlsl"

// --------------
// Pixel Shader
// --------------
struct DS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;
    float3 viewBinormal : BINORMAL;
    
    uint instanceID : InstanceID;
};

struct PS_OUT
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
};

PS_OUT PS_Main(DS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;;
    RENDERPARAMS instance = InstanceParams[idx];
    MATERIALINFO material = Materials[instance.MaterialInfoIndex];
    
    matrix WV = mul(Objects[instance.ObjectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);
    
    float4 color = float4(1.f, 1.f, 1.f, 1.f);
    
    color = TextureMaps[material.DiffuseMap0Index].Sample(g_sam_0, input.uv);

    float3 viewNormal =  input.viewNormal;
  

    float3 tangentSpaceNormal = TextureMaps[material.DiffuseMap1Index].Sample(g_sam_0, input.uv).xyz;

    tangentSpaceNormal = (tangentSpaceNormal - 0.5f) * 2.f;
    float3x3 matTBN = { input.viewTangent, input.viewBinormal, input.viewNormal };
    viewNormal = -normalize(mul(tangentSpaceNormal, matTBN));
    

    
    output.position = float4(input.viewPos.xyz, 0.f);
    

    output.normal = float4(viewNormal.xyz, 0.f);
    output.color =  float4(color.rgb, 1.0f);
    output.color *= 2.0f;

    return output;
}
