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
    float4 normal   : SV_Target1; // xyz = view normal, w = metallic
    float4 color    : SV_Target2; // rgb = baseColor,   a = roughness
    float4 emissive : SV_Target3; // rgb = emissive
};



PS_OUT PS_Main(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    uint materialIndex = instance.MaterialInfoIndex;
    MATERIALINFO materials = Materials[materialIndex];


    float4 baseColor = materials.Diffuse;

    if (materials.DiffuseMap0Index != -1)
    {
        float4 tex0 = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);
        baseColor *= tex0;
    }

 
    if (materials.DiffuseMap1Index != -1)
    {
        float4 tex1 = TextureMaps[materials.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex1, tex1.a);
    }

    if (materials.DiffuseMap2Index != -1)
    {
        float4 tex2 = TextureMaps[materials.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex2, tex2.a);
    }

    if (materials.DiffuseMap3Index != -1)
    {
        float4 tex3 = TextureMaps[materials.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex3, tex3.a);
    }

        
    if (baseColor.a < 0.01f)
    {
        discard;
    }
    
  
    float3 viewNormal = normalize(input.viewNormal);
    
    if (materials.NormalMapIndex != -1)
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
    else if (materials.DiffuseMap0Index != -1 &&
             dot(input.viewTangent, input.viewTangent) > 1e-6f)
    {
        // 가짜 bump
        const float bumpStrength = 0.2f;

        float2 texSize;
        TextureMaps[materials.DiffuseMap0Index].GetDimensions(texSize.x, texSize.y);
        float2 texel = 1.0f / max(texSize, float2(1.0f, 1.0f));

        const float3 lumaW = float3(0.299f, 0.587f, 0.114f);
        float hC = dot(TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv).rgb, lumaW);
        float hR = dot(TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv + float2(texel.x, 0.0f)).rgb, lumaW);
        float hU = dot(TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv + float2(0.0f, texel.y)).rgb, lumaW);

        // 밝기 기울기 이용 tangent-space 노멀
        float3 bumpN = normalize(float3(-(hR - hC) * bumpStrength,
                                        -(hU - hC) * bumpStrength,
                                        1.0f));

        float3x3 matTBN = float3x3(
            normalize(input.viewTangent),
            normalize(input.viewBinormal),
            normalize(input.viewNormal)
        );

        viewNormal = normalize(mul(bumpN, matTBN));
    }


    float metallic = materials.Metallic;

    float roughness = materials.Roughness;


    if (materials.MetallicMapIndex != -1)
    {
        metallic = TextureMaps[materials.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    }

    // RoughnessMap 우선, 없으면 SpecularcMap 슬롯(FBX Roughness 채널) 폴백
    if (materials.RoughnessMapIndex != -1)
    {
        roughness = TextureMaps[materials.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    }

    metallic = saturate(metallic);
    roughness = saturate(roughness);

    float3 emissive = materials.Emission;
    if(materials.EmissiveMapIndex != -1)
    {
        emissive = TextureMaps[materials.EmissiveMapIndex].Sample(g_sam_0, input.uv).rgb;
    }

    // OcclusionMap(AO) → emissive.a에 패킹해 G-Buffer로 전달
    float materialAO = 1.0f;
    if (materials.OcclusionMapIndex != -1)
    {
        materialAO = TextureMaps[materials.OcclusionMapIndex].Sample(g_sam_0, input.uv).r;
    }

    output.position = float4(input.viewPos.xyz, 1.0f);
    output.normal   = float4(viewNormal.xyz, metallic);
    output.color    = float4(baseColor.rgb, roughness);
    output.emissive = float4(emissive, materialAO); // a채널 = material AO

    return output;
}
