// ============================================================
//  ForwardPlus_ToonPS.hlsl
//
//  MATERIALINFO에 추가해야 할 필드 (TODO):
//   float4  ShadowColor;        // 그림자 색 tint (default: 0.5, 0.55, 0.65, 1)
//   float   ShadowThreshold;    // 명암 경계 위치 [0~1]     (default: 0.5)
//   float   ShadowSmoothness;   // 명암 경계 폭  [0~0.2]   (default: 0.05)
//   float4  RimColor;           // Rim 색상                  (default: white)
//   float   RimPower;           // Rim Fresnel 지수          (default: 4.0)
//   float   RimThreshold;       // Rim 컷오프 임계값         (default: 0.3)
//   float   SpecShininess;      // Toon 스펙큘러 지수        (default: 50.0)
//   float   SpecThreshold;      // Toon 스펙큘러 컷오프      (default: 0.6)
//   int     LightMapIndex;      // RGBA 라이트맵 텍스처 인덱스
// ============================================================

#include "params.hlsl"
#include "utils.hlsl"

#define FORWARD_PLUS_TILE_SIZE          16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128


float3 BlendNormalSimple(float3 nA, float3 nB)
{

    return normalize(float3(nA.rg + nB.rg, nA.b * nB.b));
}

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
//  PS_Main  (Anime Toon Forward+)
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
    
    float3 emissive = mtl.Emission;
    if(mtl.EmissiveMapIndex >= 0)
    {
        emissive = TextureMaps[mtl.EmissiveMapIndex].Sample(g_sam_0, input.uv).rgb;
        color.rgb *= emissive * abs((frac(PassParams.TotalTime * 0.5) - 0.5) * 2);
        // 깜빡이는 효과
        return color;
    }
    
    ////////////////////////////////////////////////////////////////////////

    // LightMap 샘플링
    //  원신 스타일 RGBA LightMap 구조:
    //   R : Specular Mask      (스펙큘러 강도, 1=반짝임 있음)
    //   G : Shadow Threshold   (재질마다 다른 명암 경계 오프셋)
    //   B : Specular Type      (0~0.33: Skin/Hair, 0.67~1: Metal)
    //   A : Rim Mask           (Rim이 나타날 영역 마스크)
    //
    //      MATERIALINFO에 LightMapIndex 필드 추가 필요
    float4 lightMap = float4(1.f, 0.5f, 0.9f, 0.3f); // 기본값 (LightMap 없을 때)
    if (mtl.ExtTex[1] >= 0)
        lightMap = TextureMaps[mtl.ExtTex[1]].Sample(g_sam_0, input.uv);

    float specMask = lightMap.r; // 스펙큘러 강도 마스크
    float responseMask = lightMap.b; // 메탈 반사 응답 보정
    float shadowBias = (lightMap.g - 0.5f) * 0.15f; // G채널 : shadow threshold 오프셋
    float rimMask = lightMap.a; // Rim 마스크
 

    
    
    float3 dirLightDIr = float3(0, 0, 1);
    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
        if (Lights[lightIndex].lightType == 0) // directional
        {
            float3 viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
            dirLightDIr = normalize(-viewLightDir); // surface -> light
            break;
        }
    }
    
    float3 V = normalize(-input.viewPos);
    float3 N = normalize(viewNormal);
    float3 H = normalize(V + dirLightDIr);
    float nDotLMain = saturate(dot(N, dirLightDIr));
    float nDotHMain = saturate(dot(N, H));
    
    ////////////////////////////////////////////////////////////////////////
    // Metallic / Roughness / AO
    float3 metallicGrad = float3(0, 0, 0);
    float metallic = mtl.Metallic;
    float3 metalN = viewNormal;
    if (mtl.MetallicMapIndex >= 0)
    {
        metallic = TextureMaps[mtl.MetallicMapIndex].Sample(g_sam_0, input.uv).r;


        //if (mtl.NormalMapIndex >= 0)
        //{
        //    float3 nMetalTS = TextureMaps[mtl.NormalMapIndex].Sample(g_sam_0, input.uv).xyz * 2.0f - 1.0f;
        //    float3x3 tbn = float3x3(
        //    normalize(input.viewTangent),
        //    normalize(input.viewBinormal),
        //    normalize(input.viewNormal));

        //    float3 nMetalVS = normalize(mul(nMetalTS, tbn));
        //    metalN = BlendNormalSimple(viewNormal, nMetalVS);
        //}

        //// 메인 라이트 방향(첫 directional 기준)
       

        //// 이미지 코드의 MetallicUV = dot(N, normalize(V + L))
        //float metallicUV = saturate(dot(N, normalize(V + dirLightDIr)));

        //// gradient tex 샘플 (ExtTex[2])
        //if (mtl.ExtTex[0] >= 0)
        //{
        //    metallicGrad = TextureMaps[mtl.ExtTex[0]].Sample(g_sam_Terrain, float2(metallicUV, 0.5f)).rgb;
        //}
    }
        
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
    
    
    ToonShadingParams toon;
    // ShadowThreshold: NdotL 기준 경계값 (0.5 = 빛 방향 60도 이하에서 shadow)
    // shadowBias: LightMap G채널로 머티리얼마다 경계 미세 조정 (-0.075 ~ +0.075)
    toon.ShadowThreshold = 0.5f + shadowBias;
    // ShadowSmoothness: 0.0 = 완전 하드, 0.02 = 매우 얇은 소프트 (앤티얼라이싱 수준)
    // 너무 크면 PBR처럼 보임
    toon.ShadowSmoothness = 0.1f;

    // Metallic: 높을수록 그림자 색이 청회색  베이스컬러 색조로 이동 (금속 느낌)
    toon.ShadowTint = lerp(float3(0.20f, 0.25f, 0.35f), color.rgb * 0.4f, metallic);

    // Roughness: 낮을수록 날카로운 점 하이라이트 / 높을수록 넓고 약한 하이라이트
    toon.SpecShininess = lerp(200.0f, 5.0f, roughness);
    toon.SpecThreshold = lerp(0.70f, 0.30f, roughness);

    // Metallic: 높을수록 스펙큘러 강하게 (LightMap R채널과 합산)
    toon.SpecMask = specMask;
    toon.RampTexIdx = mtl.ExtTex[0];
    
    
     ////////////////////////////////////////////////////////////////////////

    // Light Loop (Forward+ Tile에 포함된 라이트만 계산)
    LightColor totalColor = (LightColor) 0.f;
    bool directionalShadowApplied = false;
    float mainShadowFactor = 0.f; // Rim Light 계산에서 재사용

    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
        const LightColor lc = CalculateLightColorToon(
            lightIndex, viewNormal, input.viewPos, color.rgb, toon);

        

        totalColor.diffuse += lc.diffuse;
        totalColor.ambient += lc.ambient;
        totalColor.specular += lc.specular;

        //// ── CSM Shadow (DirectionalLight에만 적용) ──
        //if (!directionalShadowApplied && Lights[lightIndex].lightType == 0)
        //{
        //    const float visibility = CalculateCSMShadow(
        //        input.viewPos, viewNormal, Lights[lightIndex].direction.xyz);

        //    totalColor.diffuse *= visibility;
        //    totalColor.ambient *= visibility;
        //    // [주의] specular에도 CSM shadow 적용 (그림자 안에서 하이라이트 방지)
        //    totalColor.specular *= visibility;
        //    directionalShadowApplied = true;

        //    // [신규] Rim Light용 mainShadowFactor 계산
        //    // Directional Light의 NdotL을 기반으로 lit/shadow 면 판별
        //    float3 viewLightDir = normalize(mul(
        //        float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        //    float ndotL01 = dot(-viewLightDir, normalize(viewNormal)) * 0.5f + 0.5f;
        //    float hs = toon.ShadowSmoothness * 0.5f;
        //    mainShadowFactor = smoothstep(
        //        toon.ShadowThreshold - hs,
        //        toon.ShadowThreshold + hs,
        //        ndotL01) * visibility;
        //}
    }

    ////////////////////////////////////////////////////////////////////////
  

    ////////////////////////////////////////////////////////////////////////
    // Metal

  //  float4 nprSpecParam = float4(
  //    1.0f - roughness, // r: Glossiness (roughness가 낮을수록 날카로운 하이라이트)
  //    saturate(metallic + 0.1f), // g: 스펙큘러 색상 강도 (LightMap 없으므로 metallic 기반)
  //    metallic, // b: 금속광 강도
  //    0.0f);

  //  float3 worldNormal = normalize(mul(float4(N, 0.f), PassParams.MatViewInv).xyz);

  //  float3 nprSpec = NPR_Base_Specular(
  //    nDotLMain,
  //    nDotHMain,
  //    worldNormal,
  //    color.rgb,
  //    nprSpecParam
  //);
    
    float3 nprSpec = CalculateToonSpecular3(
      nDotLMain, // NdotL
      nDotHMain, // NdotH
      N, // viewNormal (뷰 공간, 이미 normalize된 것)
      color.rgb, // baseColor
      metallic, // metallicMask   
      roughness, // roughnessMask 
      metallic, // responseMask   (LightMap 없으므로 metallic으로 대체)
      1.0f, // metalIntensity (금속광 전체 강도, 튜닝값)
      0.7f, // specThreshold  (낮을수록 하이라이트 넓어짐, 0.4~0.7)
      0.05f // specSoftness   (경계 부드러움, 0.02~0.1)
  );

    // 기존 spec 누적에 추가
    totalColor.specular.xyz += nprSpec; //+ (metallicGrad * metallic);
    ////////////////////////////////////////////////////////////////////////
      // Rim 파라미터
    const float3 rimColor = float3(1.0f, 1.0f, 1.0f); // rim 색상
    const float rimWidth = 4.0f; // 오프셋 픽셀 수 (1~4 튜닝)
    const float rimDepthThres = 0.005f; // depth 차이 임계값 (0.005~0.03 튜닝)
    const float rimFeather = 1.0f; // 0.5~2.0
    const float rimIntensity = 1.5f; // 0.5~2.0

    // Rim Light 적용 (Depth-based Screen-Space)
    //float3 rim = CalculateRimDepthLight(
    //    input.pos.xy,
    //    viewNormal,
    //    rimColor,
    //    rimWidth,
    //    rimDepthThres,
    //    rimMask,
    //    mainShadowFactor);
   // float3 rim = CalculateFresnelRimLight(input.viewPos, input.viewNormal, rimColor, toon.SpecThreshold, rimMask);
    

    float viewDepth = max(-input.viewPos.z, 1e-3f);
    
    float3 rim = CalculateRimLightSS(
    input.pos.xy, // screenPos
    viewNormal, // viewNormal
    V, // viewDir
    dirLightDIr, // mainLightDirVS
    viewDepth, // viewDepth
    rimColor, // rimColor
    rimWidth, // rimWidth
    rimFeather, // rimFeather
    rimIntensity, // rimIntensity
    rimMask, // rimMask (lightMap.a)
    mainShadowFactor); // shadowFactor
    
    
    
    ////////////////////////////////////////////////////////////////////////
    //   Toon 합성:
    //   result = diffuse + specular + rim
    color.xyz = totalColor.diffuse.xyz
          + totalColor.specular.xyz
          + rim;

    
    return color;

}
