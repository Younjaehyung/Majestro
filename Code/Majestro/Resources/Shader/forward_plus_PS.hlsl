#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

#define FORWARD_PLUS_TILE_SIZE          16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128


// ============================================================
//  VS_OUT
// ============================================================
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

// ============================================================
//  PS_Main
// ============================================================
float4 PS_Main(VS_OUT input) : SV_Target
{
    // Forward+ Tile 인덱스 계산
    const uint2 pixelCoord = uint2(input.pos.xy);
    const uint tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
    const uint2 tileCoord = pixelCoord / FORWARD_PLUS_TILE_SIZE;
    const uint tileIndex = tileCoord.y * tileCountX + tileCoord.x;
    const uint2 tileMeta = ForwardPlusTileMeta[tileIndex];

    const uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS instance = InstanceParams[idx];
    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];

    ////////////////////////////////////////////////////////////////////////
   
    
    
    float objectAlpha = Objects[instance.ObjectIndex].Extra.x;

    
    ApplyIGNDitherFade(input.pos.xy, objectAlpha);
    

    ////////////////////////////////////////////////////////////////////////
    // Diffuse 텍스처
    float4 color = mtl.Diffuse;
    if (mtl.DiffuseMap0Index >= 0)
        color *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv);
    if (mtl.DiffuseMap1Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (mtl.DiffuseMap2Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (mtl.DiffuseMap3Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (color.a < 0.01f)
        discard;

    float3 albedo = color.rgb;

    // Normal Map
    float3 viewNormal = normalize(input.viewNormal);
    if (mtl.NormalMapIndex >= 0)
    {
        float3 n = TextureMaps[mtl.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
        n = n * 2.0f - 1.0f;
        float3x3 tbn = float3x3(
            normalize(input.viewTangent),
            normalize(input.viewBinormal),
            normalize(input.viewNormal));
        viewNormal = normalize(mul(n, tbn));
    }

    // Emissive
    float3 emissive = mtl.Emission;
    if (mtl.EmissiveMapIndex >= 0)
    {
        emissive = TextureMaps[mtl.EmissiveMapIndex].Sample(g_sam_0, input.uv).rgb;
        if (any(emissive.rgb))
        {
            color.rgb *= emissive * max(1.0f, abs((frac(PassParams.TotalTime * 0.5) - 0.5) * 2) * 10.f);
            // color.rgb *= emissive * abs((frac(PassParams.TotalTime * 0.5) - 0.5) * 2);
            return color;
        }
            

    }

    ////////////////////////////////////////////////////////////////////////
    // ILM/LightMap 샘플링
    // R: Specular Mask
    // G: AO/ShadowBias  (0=어둡게, 0.5=중립, 1=밝게)
    // B: Material Class (0.0 Hard Default / 0.2 Soft Matte /  0.4 Metal / 0.6 Silk, Stocking / 0.8 Hair / 1.0 Skin)
    // A: Rim Mask

    float4 lightMap = float4(1.f, 0.5f, 0.0f, 0.3f); // 기본값: 단단한 물체
    if (mtl.ExtTex[1] >= 0)
        lightMap = TextureMaps[mtl.ExtTex[1]].Sample(g_sam_0, input.uv);
    lightMap.r = 1.0f;
    //lightMap.g = .5f;

    lightMap.a = .3f;
    
    float specMask   = lightMap.r;

    uint  matClass   = ClassifyMaterial(lightMap.b);    // 재질 분류 (0~4)
    float ilmAO = lightMap.g;
    float shadowBias = (lightMap.g - 0.5f) * 0.15f; // G채널: shadow 경계 미세조정
    float rimMask    = lightMap.a;

    ////////////////////////////////////////////////////////////////////////
    // PBR 파라미터
    float metallic = mtl.Metallic;
    if (mtl.MetallicMapIndex >= 0)
        metallic = TextureMaps[mtl.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    metallic = saturate(metallic);

    float roughness = mtl.Roughness;
    if (mtl.RoughnessMapIndex >= 0)
        roughness = TextureMaps[mtl.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    roughness = saturate(roughness);

    float ao = 1.0f;
    if (mtl.OcclusionMapIndex >= 0)
        ao = TextureMaps[mtl.OcclusionMapIndex].Sample(g_sam_0, input.uv).r;
    ao = saturate(ao);

    ////////////////////////////////////////////////////////////////////////
    // PBR+NPR  ExtValue[0]
    //   ExtValue[0].x = ShadowOffset   (0.5 기본 — Sigmoid 경계 위치)
    //   ExtValue[0].y = ShadowSmooth   (0.08 기본 — 경계 부드러움)
    //   ExtValue[0].z = ShadowStrength (1.0 기본 — 그림자 강도)
    //   ExtValue[0].w = IBLScale       (0.15 기본 — 환경광 약화 강도)
    // ============================================================
    //   ExtTex[0] = Shadow Ramp Texture (2줄)
    //                 Row0 (V=0.25): 난반사 그림자 색상
    //                 Row1 (V=0.75): 정반사 스타일 색상
    // ============================================================
    
    
    float4 nprParam = mtl.ExtValue[0];

    PBRNPRShadingParams npr;
    npr.ShadowOffset   = (nprParam.x > 0.001f ? nprParam.x : 0.5f) + shadowBias;
    npr.ShadowSmooth   = nprParam.y > 0.001f ? nprParam.y : 0.08f;
    npr.ShadowStrength = nprParam.z > 0.001f ? nprParam.z : 1.0f;
    npr.IBLScale       = nprParam.w > 0.001f ? nprParam.w : 0.15f;
    npr.ShadowColor    = lerp(float3(0.20f, 0.25f, 0.35f), albedo * 0.4f, metallic);
    npr.SecShadowColor = npr.ShadowColor * 0.7f;
    npr.SpecMask       = specMask;
    npr.ilmAO          = ilmAO;
    npr.RampTexIdx     = mtl.ExtTex[0];
    npr.MatClass       = matClass;
    npr.ViewTangent    = normalize(input.viewTangent);
    npr.ViewBinormal   = normalize(input.viewBinormal);

    ////////////////////////////////////////////////////////////////////////
    // 메인 방향광 방향 추출
    float3 dirLightDir = float3(0, 0, 1);
    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
        if (Lights[lightIndex].lightType == 0)
        {
            float3 viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
            dirLightDir = normalize(-viewLightDir);
            break;
        }
    }

    float3 N = normalize(viewNormal);
    float3 V = normalize(-input.viewPos);

    ////////////////////////////////////////////////////////////////////////
    // PBR+NPR
    LightColor totalColor = (LightColor) 0.f;
    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
        //const LightColor lc = CalculateLightColorPBRNPR1(
        //    lightIndex, N, input.viewPos, albedo, metallic, roughness, npr);
        const LightColor lc = CalculateLightColorPBRNPR(
            lightIndex, N, input.viewPos, albedo, metallic, roughness, npr);

        totalColor.diffuse += (lc.diffuse );
    }

    ////////////////////////////////////////////////////////////////////////
    // 재질별 Rim / IBL 스케일
    float matRimScale;
    float matIBLSpecScale;
    [branch] switch (matClass)
    {
        case MAT_SKIN:  matRimScale = 1.8f; matIBLSpecScale = 0.3f; break;
        case MAT_SILK:  matRimScale = 1.3f; matIBLSpecScale = 0.8f; break;
        case MAT_METAL: matRimScale = 0.4f; matIBLSpecScale = 2.0f; break;
        case MAT_SOFT:  matRimScale = 0.8f; matIBLSpecScale = 0.6f; break;
        default:        matRimScale = 1.0f; matIBLSpecScale = 1.0f; break;
    }

    ////////////////////////////////////////////////////////////////////////
    // Rim Light
    const float3 rimColor = float3(1.0f, 1.0f, 1.0f);
    const float rimWidth = 4.0f;
    const float rimFeather = 1.0f;
    const float rimIntensity = 1.5f * matRimScale;
    float viewDepth = max(-input.viewPos.z, 1e-3f);

    float3 rim = CalculateRimLightSS(
        input.pos.xy, N, V, dirLightDir, viewDepth,
        rimColor, rimWidth, rimFeather, rimIntensity, rimMask, 0.0f);

    ////////////////////////////////////////////////////////////////////////
    // IBL 환경광
    IBLResult ibl = CalculateIBLAmbient(N, V, albedo, metallic, roughness);

    color.xyz = totalColor.diffuse.xyz
              + (ibl.diffuse * albedo * npr.IBLScale * ao)
              + (ibl.specular * npr.IBLScale * /*matIBLSpecScale **/ ao)
              + rim;

    return color;
}
