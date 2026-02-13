#include "params.hlsl"

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

struct PS_OUT
{
    float4 position : SV_Target0; // view-space pos
    float4 normal : SV_Target1; // xyz = view normal, w = metallic   [수정]
    float4 color : SV_Target2; // rgb = baseColor,   a = roughness  [수정]
};

PS_OUT PS_Main(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    uint materialIndex = instance.MaterialInfoIndex;
    MATERIALINFO materials = Materials[materialIndex];


    float4 baseColor = materials.Diffuse;

    if (materials.DiffuseMap0Index != 0)
    {
        float4 tex0 = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);
        baseColor *= tex0;
    }

 
    if (materials.DiffuseMap1Index != 0)
    {
        float4 tex1 = TextureMaps[materials.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex1, tex1.a);
    }

    if (materials.DiffuseMap2Index != 0)
    {
        float4 tex2 = TextureMaps[materials.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex2, tex2.a);
    }

    if (materials.DiffuseMap3Index != 0)
    {
        float4 tex3 = TextureMaps[materials.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex3, tex3.a);
    }

        
    if (baseColor.a < 0.01f)
    {
        discard;
    }
    
  
    float3 viewNormal = normalize(input.viewNormal);
    
    if (materials.NormalMapIndex != 0)
    {
        float3 tangentSpaceNormal = TextureMaps[materials.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;

        // [0,1] -> [-1,1]
        tangentSpaceNormal = tangentSpaceNormal * 2.0f - 1.0f;

        float3x3 matTBN = float3x3(
            normalize(input.viewTangent),
            normalize(input.viewBinormal),
            normalize(input.viewNormal)
        );

        // tangent -> view
        viewNormal = normalize(mul(tangentSpaceNormal, matTBN));
    }


    float metallic = materials.Metallic;

    float roughness = materials.Roughness;


    if (materials.MetallicMapIndex != 0)
    {
        
        metallic = TextureMaps[materials.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    }
    metallic = 0.0f;

  
    if (materials.RoughnessMapIndex != 0)
    {
        roughness = TextureMaps[materials.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    }
    roughness = 0.5f;
        

    metallic = saturate(metallic);
    roughness = saturate(roughness);


    output.position = float4(input.viewPos.xyz, 1.0f);


    output.normal = float4(viewNormal.xyz, metallic);


    output.color = float4(baseColor.rgb, roughness);

    return output;
}
