#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"



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
    nointerpolation uint materialIndex : MaterialIndex;
    nointerpolation float4 objectExtra : ObjectExtra;
    nointerpolation float4 objectHighlight : ObjectHighlight;
};

// ============================================================
//  PS_Main
// ============================================================
float4 PS_Main(VS_OUT input) : SV_Target
{
    // Forward+ Tile 인덱스 계산

    // 인스턴스와 오브젝트 정보를 픽셀마다 다시 조회하지 않는다.
    const MATERIALINFO mtl = Materials[input.materialIndex];

    ////////////////////////////////////////////////////////////////////////
   
    
    
    float objectAlpha = input.objectExtra.x;
    // Hit Flash: 0~1, 최종 컬러를 빨강으로 lerp하는 가중치
    float hitFlash    = saturate(input.objectExtra.y);
    // Emissive Gate: 0~1, 발광 결과
    float emissiveGate = input.objectExtra.z;


    ApplyIGNDitherFade(input.pos.xy, objectAlpha);

    // ── 디졸브 소멸 연출 (적 사망) ─────────────────────────────
    // Extra.w = 진행도(0=정상, 1=완전 소멸). 노이즈 경계 기준으로 clip하고
    // 경계 근처는 발광색을 더해 타들어가는 느낌을 준다.
    const float3 kDissolveGlowColor = float3(3.0f, 0.9f, 0.15f); // HDR 주황 발광
    const float  kDissolveEdgeWidth = 0.10f;
    float dissolve = saturate(input.objectExtra.w);
    float dissolveEdge = 0.0f;
    if (dissolve > 0.0f)
    {
        float dnoise = SampleDissolveNoise(mtl.ExtTex[2], input.uv); // 머티리얼 노이즈 텍스처 우선, 없으면 절차적
        if (dnoise < dissolve)
            discard; // 이미 소멸한 영역
        // 경계(노이즈 값이 임계치에 가까운 곳)일수록 1
        dissolveEdge = 1.0f - saturate((dnoise - dissolve) / kDissolveEdgeWidth);
    }



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


    float3 emissive = mtl.Emission;
    if (mtl.EmissiveMapIndex >= 0)
    {
        emissive = TextureMaps[mtl.EmissiveMapIndex].Sample(g_sam_0, input.uv).rgb;
        if (any(emissive.rgb))
        {
            color.rgb *= emissive * max(1.0f, abs((frac(PassParams.TotalTime * 0.5) - 0.5) * 2) * 10.f * emissiveGate);
            // color.rgb *= emissive * abs((frac(PassParams.TotalTime * 0.5) - 0.5) * 2);
            color.rgb = lerp(color.rgb, float3(1.0f, 0.0f, 0.0f), hitFlash);
            color.rgb += kDissolveGlowColor * dissolveEdge;
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
    // 단일 방향광은 타일 목록 조회 없이 직접 사용한다.
    float3 viewLightDir = normalize(
        mul(float4(PassParams.DirectionalLight.direction.xyz, 0.f),
            PassParams.MatView).xyz);
    float3 dirLightDir = normalize(-viewLightDir);

    float3 N = normalize(viewNormal);
    float3 V = normalize(-input.viewPos);

    ////////////////////////////////////////////////////////////////////////
    // PBR+NPR
    LightColor totalColor = CalculateLightColorPBRNPR(
        N, input.viewPos, albedo, metallic, roughness, npr);

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

    // Hit Flash: 피격 시 그림자 속에서도 보이도록 라이팅 결과 위에 빨강으로 lerp
    color.rgb = lerp(color.rgb, float3(1.0f, 0.0f, 0.0f), hitFlash);
    color.rgb += kDissolveGlowColor * dissolveEdge;

    // 궁극기
    float hlIntensity = input.objectHighlight.a;
    if (hlIntensity > 0.001f)
    {
        float3 hlColor = input.objectHighlight.rgb;
        float f = 1.0f - saturate(dot(N, V)); // 실루엣 (가장자리일수록 1.0, 정면일수록 0.0)


        float softHalo = pow(f, 1.5f);
        float hotRim   = pow(f, 5.0f);

        float4 wp = mul(float4(input.viewPos, 1.0f), PassParams.MatViewInv);
        float worldY = wp.y / wp.w;
        float band = sin(worldY * 0.045f - PassParams.TotalTime * 3.5f) * 0.5f + 0.5f;
        band *= band; 
        
        float aura = softHalo * (0.35f + 0.65f * band) + hotRim * 1.5f;
        color.rgb += hlColor * hlIntensity * aura;
    }

    return color;
}
