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


    float metallic = materials.Metallic;

    float roughness = materials.Roughness;


    if (materials.MetallicMapIndex != -1)
    {
        metallic = TextureMaps[materials.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    }

    if (materials.RoughnessMapIndex != -1)
    {
        roughness = TextureMaps[materials.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    }


    metallic = saturate(metallic);
    roughness = saturate(roughness);


    output.position = float4(input.viewPos.xyz, 1.0f);


    output.normal = float4(viewNormal.xyz, metallic);


    // ── 에미시브 처리 ──────────────────────────────────────────────────
    // 에미시브 세기를 output.color.a (roughness 슬롯)에 저장.
    // final_PS.hlsl에서 albedo * lightPower 와 별도로 더해
    // 조명에 영향받지 않는 순수 발광 값이 HDR SceneColor에 도달하도록 함.
    //
    //   Gbuffer[3] 레이아웃:
    //     .rgb = baseColor
    //     .a   = roughness (에미시브 없는 경우)
    //          = -roughness (에미시브 있는 경우 — 부호 플래그)
    //           에미시브 세기는 별도 채널이 없으므로 materials.Emission.x를 roughness 자리에 패킹
    //
    // 주의: 에미시브가 있는 픽셀은 roughness를 에미시브 강도로 덮어쓴다.
    //        final_PS.hlsl에서 Gbuffer[3].a < 0 이면 에미시브 픽셀로 판별.
    // ─────────────────────────────────────────────────────────────────────
    float3 emissiveColor = float3(0, 0, 0);
    float  emissiveStrength = 1000.f;

    if (materials.EmissiveMapIndex != -1)
    {
        // 에미시브 맵: [0,1] LDR 텍스처 → HDR 에너지로 스케일업
        // 스케일 배율(100)은 머티리얼의 Emission.x로 제어 가능
        float scale = (materials.Emission.x > 0.f) ? materials.Emission.x : 100.f;
        emissiveColor    = TextureMaps[materials.EmissiveMapIndex].Sample(g_sam_0, input.uv).rgb * scale;
        emissiveStrength = dot(emissiveColor, float3(0.2126f, 0.7152f, 0.0722f)); // luminance
    }

    // .a = roughness (양수) 또는 에미시브 세기 (음수 플래그로 구분)
    // final_PS에서 abs(a) = roughness, sign(a) < 0 = 에미시브 픽셀
    output.color = float4(baseColor.rgb, roughness);

    if (emissiveStrength > 0.f)
    {
        // 에미시브 픽셀: rgb에 에미시브 색 저장, a를 음수로 플래깅
        // (baseColor는 lighting pass에서 사용 안 함 — 에미시브가 덮어씀)
        output.color = float4(emissiveColor, -roughness - 0.001f);
    }

    return output;
}
