////#include "params.hlsl"
////#include "utils.hlsl"

////#define FORWARD_PLUS_TILE_SIZE 16
////#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128

////struct VS_OUT
////{
////    float4 pos : SV_Position;
////    float2 uv : TEXCOORD;
////    float3 viewPos : POSITION;
////    float3 viewNormal : NORMAL;
////    float3 viewTangent : TANGENT;
////    float3 viewBinormal : BINORMAL;
////    uint instanceID : InstanceID;
////};

////// dir/view 공간 일관성은 네 엔진 전제에 맞춰야 함.
////// 여기서는 input.viewPos / viewNormal을 쓰므로 lightDir도 view space라고 가정.
////float4 PS_Main(VS_OUT input) : SV_Target
////{
////    const uint idx = GlobalParams.BaseInstanceID + input.instanceID;
////    const RENDERPARAMS instance = InstanceParams[idx];
////    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];

////    float4 color = mtl.Diffuse;

////    if (mtl.DiffuseMap0Index >= 0)
////        color *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv);

////    if (mtl.DiffuseMap1Index >= 0)
////    {
////        float4 t = TextureMaps[mtl.DiffuseMap1Index].Sample(g_sam_0, input.uv);
////        color = lerp(color, t, t.a);
////    }
////    if (mtl.DiffuseMap2Index >= 0)
////    {
////        float4 t = TextureMaps[mtl.DiffuseMap2Index].Sample(g_sam_0, input.uv);
////        color = lerp(color, t, t.a);
////    }
////    if (mtl.DiffuseMap3Index >= 0)
////    {
////        float4 t = TextureMaps[mtl.DiffuseMap3Index].Sample(g_sam_0, input.uv);
////        color = lerp(color, t, t.a);
////    }
////    if (color.a < 0.01f)
////        discard;

////    float3 viewNormal = normalize(input.viewNormal);
////    if (mtl.NormalMapIndex >= 0)
////    {
////        float3 n = TextureMaps[mtl.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
////        n = n * 2.0f - 1.0f;
////        float3x3 tbn = float3x3(
////            normalize(input.viewTangent),
////            normalize(input.viewBinormal),
////            normalize(input.viewNormal));
////        viewNormal = normalize(mul(n, tbn));
////    }

////    const uint2 pixelCoord = uint2(input.pos.xy);
////    const uint tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
////    const uint2 tileCoord = pixelCoord / FORWARD_PLUS_TILE_SIZE;
////    const uint tileIndex = tileCoord.y * tileCountX + tileCoord.x;
////    const uint2 tileMeta = ForwardPlusTileMeta[tileIndex];

////    // [수정] PBR 누적 구조 대신 Toon 누적 구조로 변경
////    float3 outDiffuse = 0.0f;
////    float3 outSpec = 0.0f;
////    float3 outAmbient = 0.0f;

////    // [추가] view space에서 카메라 원점 가정: V = pixel -> camera
////    float3 V = normalize(-input.viewPos);

////    bool keyDirectionalDone = false;
////    float keyVisibility = 1.0f; // [추가] key light shadow visibility 저장(필요 시)

////    [loop]
////    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
////    {
////        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
////        const EvaluatedLight el = EvaluateLightVS(lightIndex, input.viewPos, viewNormal); // [수정] 라이트 평가 분리

////        // [추가] 라이트 영향 없으면 스킵(ambient는 아래에서 처리)
////        if (el.NdotL <= 0.0f || el.atten <= 0.0f)
////        {
////            // ambient는 거리감쇠만 반영(너 기존과 같은 의도)
////            outAmbient += el.ambRGB * el.atten;
////            continue;
////        }

////        if (!keyDirectionalDone && el.type == 0)
////        {
////            // -------------------------
////            // [수정] Key Directional Light: Toon + CSM
////            // -------------------------
////            // 너가 말한대로 CSM은 view space lightDir을 기대함 -> el.L을 그대로 넘김
////            const float visibility = CalculateCSMShadow(input.viewPos, viewNormal, el.L); // [수정] view space dir
////            keyVisibility = visibility;

////            const float step01 = ToonShadowStep(el.NdotL);

////            // [수정] Toon diffuse (direct light만 shadow 적용)
////            outDiffuse += ToonDiffuse(color.rgb, el.diffRGB, el.atten, step01) * visibility;

////            // [수정] Toon spec band
////            float3 H = normalize(el.L + V);
////            float band = ToonSpecBand(dot(viewNormal, H));
////            outSpec += (el.specRGB * el.atten) * (band * gSpecIntensity) * visibility;

////            // [수정] ambient는 shadow 영향 약하게/없게 유지 (전신 룩 안정)
////            outAmbient += el.ambRGB * el.atten;

////            keyDirectionalDone = true;
////        }
////        else
////        {
////            // -------------------------
////            // [추가] Other Lights: 룩 유지용 "보조" 누적
////            // -------------------------
////            // 보조 diffuse는 과하지 않게만(툰 경계는 키라이트로만 유지)
////            outDiffuse += (el.diffRGB * el.atten) * color.rgb * gOtherDiffScale;

////            // 보조 spec도 약하게만(추후 MaskMap으로 부위 선택 권장)
////            // 보조 스펙은 NdotH 밴드까지 주면 라이트 많은 장면에서 번쩍임이 심해질 수 있어 단순 스케일로 시작
////            outSpec += (el.specRGB * el.atten) * gOtherSpecScale;

////            outAmbient += el.ambRGB * el.atten * 0.5f;
////        }
////    }

////    // [수정] 최종 합성(기존 totalColor와 동일한 역할)
////    float3 finalRGB = outAmbient * color.rgb + outDiffuse + outSpec;

////    // [추가] 과포화 방지(툰은 라이트가 많으면 금방 날아감)
////    finalRGB = saturate(finalRGB);

////    return float4(finalRGB, color.a);
////}

//#include "params.hlsl"
//#include "utils.hlsl"

//#define FORWARD_PLUS_TILE_SIZE 16
//#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128

//struct VS_OUT
//{
//    float4 pos : SV_Position;
//    float2 uv : TEXCOORD;
//    float3 viewPos : POSITION;
//    float3 viewNormal : NORMAL;
//    float3 viewTangent : TANGENT;
//    float3 viewBinormal : BINORMAL;
//    uint instanceID : InstanceID;
//};

//// [추가] NiloCat 예제의 핵심: NoL을 cel 영역(0~1)으로 변환 (mid/softness)
//float ToonLitFactor(float NoL, float midPoint, float softness)
//{
//    // [중요] NiloCat: smoothstep(mid-soft, mid+soft, NoL) :contentReference[oaicite:2]{index=2}
//    return smoothstep(midPoint - softness, midPoint + softness, NoL);
//}

//// [추가] View-space 기반 단일 라이트 Toon 셰이딩
//float3 ShadeSingleLight_Toon_ViewSpace(
//    int lightIndex,
//    float3 N_view, // normalized
//    float3 viewPos, // pixel position in view space
//    float3 baseColor,
//    float3 shadowColor, // shadow tint
//    float celMid,
//    float celSoft,
//    float receiveShadowAmount,
//    bool isAdditionalLight,
//    float directionalShadowVisibility // dir light일 때만 유효(0~1), 아니면 1
//)
//{
//    float3 lightToSurface_view = 0;
//    float distanceRatio = 1.0f;
//    float NoL = 0.0f;

//    // -----------------------------
//    // 1) 기존 네 코드와 동일한 "view space" 라이트 벡터/감쇠 구성
//    // -----------------------------
//    if (Lights[lightIndex].lightType == 0)
//    {
//        // Directional
//        lightToSurface_view = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
//        float3 L = normalize(-lightToSurface_view); // surface -> light
//        NoL = dot(N_view, L);
//        distanceRatio = 1.0f;
//    }
//    else if (Lights[lightIndex].lightType == 1)
//    {
//        // Point
//        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
//        lightToSurface_view = normalize(viewPos - viewLightPos); // light -> surface
//        float3 L = normalize(-lightToSurface_view); // surface -> light
//        NoL = dot(N_view, L);

//        float dist = distance(viewPos, viewLightPos);
//        distanceRatio = (Lights[lightIndex].range <= 0.f) ? 0.f
//                        : saturate(1.f - pow(dist / Lights[lightIndex].range, 2));
//    }
//    else
//    {
//        // Spot (네 기존 구조 유지)
//        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
//        lightToSurface_view = normalize(viewPos - viewLightPos);
//        float3 L = normalize(-lightToSurface_view);
//        NoL = dot(N_view, L);

//        if (Lights[lightIndex].range <= 0.f)
//        {
//            distanceRatio = 0.f;
//        }
//        else
//        {
//            float halfAngle = Lights[lightIndex].angle * 0.5f;

//            float3 viewLightVec = viewPos - viewLightPos;
//            float3 viewCenterLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);

//            float centerDist = dot(viewLightVec, viewCenterLightDir);
//            float lightAngle = acos(dot(normalize(viewLightVec), viewCenterLightDir));

//            if (centerDist < 0.f || centerDist > Lights[lightIndex].range || lightAngle > halfAngle)
//                distanceRatio = 0.f;
//            else
//                distanceRatio = saturate(1.f - pow(centerDist / Lights[lightIndex].range, 2));
//        }
//    }

//    if (distanceRatio <= 0.0f)
//        return 0.0f;

//    // -----------------------------
//    // 2) NiloCat 방식 Toon lit/shadow 영역 만들기
//    // -----------------------------
//    float lit = ToonLitFactor(NoL, celMid, celSoft);
//    lit = saturate(lit);

//    // [수정] Directional shadow(CSM)만 예시처럼 반영
//    // NiloCat: lit *= lerp(1, light.shadowAttenuation, amount) :contentReference[oaicite:3]{index=3}
//    // 여기서는 URP의 shadowAttenuation 대신 네 CSM visibility를 사용
//    lit *= lerp(1.0f, directionalShadowVisibility, receiveShadowAmount);

//    // [수정] shadow tint 적용: lerp(shadowColor, 1, lit) :contentReference[oaicite:4]{index=4}
//    float3 litOrShadowColor = lerp(shadowColor, 1.0.xxx, lit);

//    // 라이트 색 (diffuse만 사용: “애니메 스타일” 베이스)
//    float3 lightRGB = saturate(Lights[lightIndex].color.diffuse.rgb);

//    // [수정] 추가 라이트 약화(URP 예제는 additional에 0.25) :contentReference[oaicite:5]{index=5}
//    float additionalScale = isAdditionalLight ? 0.25f : 1.0f;

//    return lightRGB * litOrShadowColor * distanceRatio * additionalScale;
//}

//float4 PS_Main(VS_OUT input) : SV_Target
//{
//    const uint idx = GlobalParams.BaseInstanceID + input.instanceID;
//    const RENDERPARAMS instance = InstanceParams[idx];
//    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];

//    float4 color = mtl.Diffuse;

//    // deferred/forward와 동일하게 0~3 모두 반영
//    if (mtl.DiffuseMap0Index >= 0)
//        color *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv);

//    if (mtl.DiffuseMap1Index >= 0)
//    {
//        float4 t = TextureMaps[mtl.DiffuseMap1Index].Sample(g_sam_0, input.uv);
//        color = lerp(color, t, t.a);
//    }
//    if (mtl.DiffuseMap2Index >= 0)
//    {
//        float4 t = TextureMaps[mtl.DiffuseMap2Index].Sample(g_sam_0, input.uv);
//        color = lerp(color, t, t.a);
//    }
//    if (mtl.DiffuseMap3Index >= 0)
//    {
//        float4 t = TextureMaps[mtl.DiffuseMap3Index].Sample(g_sam_0, input.uv);
//        color = lerp(color, t, t.a);
//    }
//    if (color.a < 0.01f)
//        discard;

//    // -----------------------------
//    // Normal (view space 유지)
//    // -----------------------------
//    float3 N_view = normalize(input.viewNormal);
//    if (mtl.NormalMapIndex >= 0)
//    {
//        float3 n = TextureMaps[mtl.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
//        n = n * 2.0f - 1.0f;
//        float3x3 tbn = float3x3(
//            normalize(input.viewTangent),
//            normalize(input.viewBinormal),
//            normalize(input.viewNormal));
//        // [주의] HLSL row/col 규약은 네 기존과 동일하게 유지
//        N_view = normalize(mul(n, tbn));
//    }

//    // -----------------------------
//    // Forward+ tile light list
//    // -----------------------------
//    const uint2 pixelCoord = uint2(input.pos.xy);
//    const uint tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
//    const uint2 tileCoord = pixelCoord / FORWARD_PLUS_TILE_SIZE;
//    const uint tileIndex = tileCoord.y * tileCountX + tileCoord.x;
//    const uint2 tileMeta = ForwardPlusTileMeta[tileIndex];

//    // -----------------------------
//    // [추가] Toon 파라미터 (지금은 상수. 나중에 MATERIALINFO/CB로 빼는 걸 추천)
//    // -----------------------------
//    // [추천] mtl에 ToonMid/ToonSoft/ShadowColor/ReceiveShadowAmount 같은 값을 넣어서 캐릭터별 튜닝 가능하게 만들 것.
//    const float celMid = 0.5f; // NiloCat _CelShadeMidPoint :contentReference[oaicite:6]{index=6}
//    const float celSoft = 0.05f; // NiloCat _CelShadeSoftness :contentReference[oaicite:7]{index=7}
//    const float3 shadowColor = float3(0.25f, 0.25f, 0.25f); // NiloCat _ShadowMapColor 개념 :contentReference[oaicite:8]{index=8}
//    const float receiveShadowAmount = 1.0f; // NiloCat _ReceiveShadowMappingAmount :contentReference[oaicite:9]{index=9}
//    const float3 indirectMin = float3(0.04f, 0.04f, 0.04f); // NiloCat _IndirectLightMinColor 개념 :contentReference[oaicite:10]{index=10}

//    float3 directSum = 0.0f;
//    float3 ambientSum = 0.0f;

//    bool directionalShadowApplied = false;
//    float dirVisibility = 1.0f;

//    [loop]
//    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
//    {
//        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];

//        // ambient는 기존 LIGHTINFO 구조를 그대로 활용
//        ambientSum += saturate(Lights[lightIndex].color.ambient.rgb);

//        // [수정] Directional shadow visibility는 한 번만 계산해서 재사용
//        if (!directionalShadowApplied && Lights[lightIndex].lightType == 0)
//        {
//            dirVisibility = CalculateCSMShadow(input.viewPos, N_view, Lights[lightIndex].direction.xyz);
//            directionalShadowApplied = true;
//        }

//        const bool isAdditional = (Lights[lightIndex].lightType != 0);

//        directSum += ShadeSingleLight_Toon_ViewSpace(
//            lightIndex,
//            N_view,
//            input.viewPos,
//            color.rgb,
//            shadowColor,
//            celMid,
//            celSoft,
//            receiveShadowAmount,
//            isAdditional,
//            (Lights[lightIndex].lightType == 0) ? dirVisibility : 1.0f
//        );
//    }

//    // -----------------------------
//    // [수정] NiloCat의 합성 느낌: "직사광 합"이 너무 커지는 걸 막으면서 hue 유지
//    // rawLightSum = max(indirect, main+additional) :contentReference[oaicite:11]{index=11}
//    // 여기서는 (ambientSum + directSum)을 direct term으로 보고, 최소 간접광(indirectMin)과 max를 취함.
//    // -----------------------------
//    float3 rawLightSum = max(indirectMin, ambientSum + directSum);
//    color.rgb = color.rgb * rawLightSum;

//    return color;
//}



// ============================================================
//  ForwardPlus_ToonPS.hlsl
//  Anime-Style Toon Shading (Genshin Impact / ZZZ Style)
//
//  참고:
//   - NiloCat UnityURPToonLitShaderExample (LightingEquation.hlsl)
//   - Genshin Impact shader breakdown (Adrian Mendez, ArtStation)
//
//  기존 PBR PS에서 변경된 핵심 포인트:
//   1. CalculateLightColorPBR  → CalculateLightColorToon
//   2. smoothstep NdotL 명암 경계 (hard cel-shading)
//   3. Shadow Color Tint (그림자 영역에 색조 적용)
//   4. Step-based Toon Specular (GGX 대신 하드컷 하이라이트)
//   5. Rim Light (Fresnel 기반 윤곽 하이라이트)
//   6. LightMap 지원 추가 (원신 스타일 RGBA 라이트맵)
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

// ============================================================
//  ToonShadingParams
//  Toon 조명 계산에 필요한 파라미터를 묶은 구조체.
//  PS_Main에서 채워서 CalculateLightColorToon에 전달.
// ============================================================
struct ToonShadingParams
{
    float ShadowThreshold; // 명암 경계 위치   [0~1]    (0.5 = 정중앙)
    float ShadowSmoothness; // 명암 경계 부드러움 [0~0.2] (0.0 = 완전 하드)
    float3 ShadowTint; // 그림자 영역 색 tint        (청회색 = 원신 기본)
    float SpecShininess; // 스펙큘러 pow 지수          (클수록 좁고 날카롭게)
    float SpecThreshold; // 스펙큘러 하드컷 위치       (0.6 = 60% 이상에서만)
    float SpecMask; // 스펙큘러 강도 마스크        (LightMap.r에서 읽음)
};

// ============================================================
//  CalculateLightColorToon
//
//  PBR(Cook-Torrance)을 대체하는 Toon 조명 함수.
//
//  [용어 정리]
//   NdotL     : Normal dot Light - 표면이 빛을 얼마나 향하는지 (1: 정면, -1: 뒤)
//   shadowFactor : smoothstep으로 NdotL을 [0,1]로 변환한 명암 비율
//   Toon Ramp : NdotL을 단계적 색상으로 변환하는 방식 (여기선 lerp로 2톤 구현)
//   Toon Spec : step()으로 하이라이트 경계를 하드컷 처리한 스펙큘러
// ============================================================
LightColor CalculateLightColorToon(
    int lightIndex,
    float3 viewNormal,
    float3 viewPos,
    float3 baseColor,
    ToonShadingParams toon)
{
    LightColor color = (LightColor) 0.f;

    float3 viewLightDir = (float3) 0.f;
    float distanceRatio = 1.f;
    float rawNdotL = 0.f; // 리매핑 전 원본 NdotL [-1, 1]

    // ─────────────────────────────────────────────────────────
    //  1) 라이트 방향 / 감쇠 계산  (기존 PBR 구조 그대로 유지)
    // ─────────────────────────────────────────────────────────
    if (Lights[lightIndex].lightType == 0)
    {
        // Directional Light
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        rawNdotL = dot(-viewLightDir, viewNormal);
    }
    else if (Lights[lightIndex].lightType == 1)
    {
        // Point Light
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        rawNdotL = dot(-viewLightDir, viewNormal);

        float dist = distance(viewPos, viewLightPos);
        distanceRatio = (Lights[lightIndex].range == 0.f)
                        ? 0.f
                        : saturate(1.f - pow(dist / Lights[lightIndex].range, 2));
    }
    else
    {
        // Spot Light
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        rawNdotL = dot(-viewLightDir, viewNormal);

        if (Lights[lightIndex].range == 0.f)
        {
            distanceRatio = 0.f;
        }
        else
        {
            float halfAngle = Lights[lightIndex].angle / 2.f;
            float3 viewLightVec = viewPos - viewLightPos;
            float3 viewCenterDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
            float centerDist = dot(viewLightVec, viewCenterDir);
            float lightAngle = acos(dot(normalize(viewLightVec), viewCenterDir));

            if (centerDist < 0.f || centerDist > Lights[lightIndex].range || lightAngle > halfAngle)
                distanceRatio = 0.f;
            else
                distanceRatio = saturate(1.f - pow(centerDist / Lights[lightIndex].range, 2));
        }
    }

    // Ambient는 라이트가 닿지 않아도 반환 (환경광이므로)
    color.ambient = Lights[lightIndex].color.ambient * distanceRatio;

    if (distanceRatio <= 0.f)
        return color;

    // ─────────────────────────────────────────────────────────
    //  2) Toon Shadow Factor 계산
    //
    //  [핵심 기법] NdotL → shadowFactor 변환
    //
    //  PBR:  NdotL을 그대로 곱해서 자연스러운 그라디언트
    //  Toon: smoothstep으로 경계를 만들어 단계적(flat) 명암 표현
    //
    //  rawNdotL [-1, 1] → [0, 1] 리매핑 후 smoothstep 적용
    //  ShadowThreshold  = 경계 위치 (0.5 = 하프램버트 기준 중간)
    //  ShadowSmoothness = 경계 폭   (0.0 = 완전 하드, 0.1 = 원신 스타일 미세 소프트)
    // ─────────────────────────────────────────────────────────
    float ndotL01 = rawNdotL * 0.5f + 0.5f; // Half-Lambert 리매핑 [-1,1] → [0,1]
    float halfSmooth = toon.ShadowSmoothness * 0.5f;

    float shadowFactor = smoothstep(
        toon.ShadowThreshold - halfSmooth,
        toon.ShadowThreshold + halfSmooth,
        ndotL01
    );

    // [원신 스타일] Outer Shadow: 약간 더 넓은 smoothstep을 min()으로 겹침
    // → 그림자 경계 직전에 약하게 pre-shadow 느낌을 줌
    float outerShadow = smoothstep(
        toon.ShadowThreshold - halfSmooth * 3.f,
        toon.ShadowThreshold + halfSmooth,
        ndotL01
    );
    shadowFactor = min(shadowFactor, outerShadow);

    // ─────────────────────────────────────────────────────────
    //  3) 2-Tone Shadow Ramp (Toon 색상 블렌딩)
    //
    //  PBR:  baseColor × NdotL (자연스러운 그라디언트)
    //  Toon: lit 영역과 shadow 영역의 색이 서로 다름
    //
    //  litColor   = baseColor × lightColor        (밝은 영역)
    //  shadowColor = baseColor × shadowTint × lightColor  (어두운 영역: 색조 변환)
    //  결과 = lerp(shadowColor, litColor, shadowFactor)
    // ─────────────────────────────────────────────────────────
    float3 lightRGB = saturate(Lights[lightIndex].color.diffuse.rgb) * distanceRatio;

    float3 litColor = baseColor; // 밝은 면: baseColor 원본 유지
    float3 shadColor = baseColor * toon.ShadowTint; // 어두운 면: tint만 적용
    float3 diffResult = lerp(shadColor, litColor, shadowFactor);
    diffResult *= lightRGB; // 라이트 색/강도를 마지막에 곱함


    // ─────────────────────────────────────────────────────────
    //  4) Toon Specular (Step-Based Hard Specular)
    //
    //  PBR:  GGX 분포함수로 부드러운 하이라이트
    //  Toon: pow(NdotH, shininess) → step()으로 하드컷
    //        → 애니 캐릭터의 날카로운 머리카락/금속 하이라이트 표현
    //
    //  [주의] 그림자 영역에서는 스펙큘러 억제 (shadowFactor 곱)
    //         → 그림자 안에서 반짝이는 부자연스러운 현상 방지
    // ─────────────────────────────────────────────────────────
    float3 N = normalize(viewNormal);
    float3 V = normalize(-viewPos); // View-Space: 카메라 방향
    float3 L = normalize(-viewLightDir); // Light 방향 (surface → light)
    float3 H = normalize(V + L); // Half Vector
    float NdotH = saturate(dot(N, H));

    // pow → step 변환이 Toon Specular의 핵심
    float specIntensity = pow(NdotH, toon.SpecShininess);
    float specStep = step(toon.SpecThreshold, specIntensity) * toon.SpecMask;
    float3 specRGB = saturate(Lights[lightIndex].color.specular.rgb);
    float3 specResult = specRGB * specStep * 0.6f * distanceRatio; // 0.6f = 강도 제한
    specResult *= shadowFactor;

    color.diffuse = float4(diffResult, 1.f);
    color.specular = float4(specResult, 1.f);

    return color;
}

// ============================================================
//  CalculateRimLight
//
//  [기법] Fresnel 기반 윤곽 하이라이트 (Rim Light)
//
//  원신/젠존제 스타일 특징:
//   - 캐릭터 윤곽선을 따라 밝은 빛이 감싸는 효과
//   - 조명 반대쪽(그림자 면) 윤곽에서 더 강하게 나타남
//   - NdotV가 낮을수록 (= 카메라에서 수직에 가까울수록) 강해짐
//
//  [용어]
//   Fresnel Effect: 표면을 비스듬히 볼수록 반사가 강해지는 광학 현상
//                   1 - NdotV 로 근사함
//   rimPower      : 지수가 클수록 rim이 얇아지고 날카로워짐
//   rimThreshold  : step으로 너무 약한 rim을 잘라냄
//
//  [파라미터]
//   shadowFactor : 0 = 그림자 면, 1 = 밝은 면
//                  그림자 면에서 rim이 더 강하게 나타남
// ============================================================
float3 CalculateRimLight(
    float3 viewNormal,
    float3 viewPos,
    float3 rimColor,
    float rimPower,
    float rimThreshold,
    float rimMask,
    float shadowFactor)
{
    float3 N = normalize(viewNormal);
    float3 V = normalize(-viewPos);
    float NdotV = saturate(dot(N, V));

    // Fresnel 근사: 1 - NdotV (엣지에서 1에 가까워짐)
    float rim = pow(1.f - NdotV, rimPower);

    // [원신 스타일] 그림자 면에서 rim이 더 강하게
    // shadowFactor=0(어두운 면)일수록 rim을 강조
    float rimBoost = lerp(1.2f, 0.4f, shadowFactor);
    rim *= rimBoost;

    // 너무 옅은 rim은 smoothstep으로 컷오프 (하드한 rim 라인 표현)
    rim = smoothstep(rimThreshold - 0.01f, rimThreshold + 0.04f, rim);

    return rimColor * rim * rimMask;
}

// ============================================================
//  VS_OUT (기존 그대로 유지)
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
    const uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS instance = InstanceParams[idx];
    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];

    // ─── Diffuse 텍스처 샘플링 (기존과 동일) ────────────────
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

    // ─── Normal Map (기존과 동일) ──────────────────────────
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

    // ─── [신규] LightMap 샘플링 ────────────────────────────
    //  원신 스타일 RGBA LightMap 구조:
    //   R : Specular Mask      (스펙큘러 강도, 1=반짝임 있음)
    //   G : Shadow Threshold   (재질마다 다른 명암 경계 오프셋)
    //   B : Specular Type      (0~0.33: Skin/Hair, 0.67~1: Metal)
    //   A : Rim Mask           (Rim이 나타날 영역 마스크)
    //
    //  [TODO] MATERIALINFO에 LightMapIndex 필드 추가 필요
    //         없으면 fallback 기본값 사용
    // ─────────────────────────────────────────────────────
    float4 lightMap = float4(0.2f, 0.5f, 0.0f, 1.0f); // 기본값 (LightMap 없을 때)
    //if (mtl.LightMapIndex >= 0)
    //    lightMap = TextureMaps[mtl.LightMapIndex].Sample(g_sam_0, input.uv);

    float specMask = lightMap.r; // 스펙큘러 강도 마스크
    float shadowBias = (lightMap.g - 0.5f) * 0.15f; // G채널 → shadow threshold 오프셋
    float rimMask = lightMap.a; // Rim 마스크

    // ─── [신규] ToonShadingParams 설정 ─────────────────────
    //  [TODO] 아래 값들은 추후 MATERIALINFO의 Toon 전용 필드에서 읽어야 함
    //         (mtl.ShadowThreshold, mtl.ShadowSmoothness, mtl.ShadowColor 등)
    //         현재는 원신 스타일 기본값으로 하드코딩

    ToonShadingParams toon;
    toon.ShadowThreshold = 0.5f + shadowBias; // LightMap G채널 보정 반영
    toon.ShadowSmoothness = 0.05f; // 원신 스타일: 약간 부드러운 경계
    toon.ShadowTint = float3(0.50f, 0.55f, 0.65f); // 청회색 tint (원신 그림자 기본 색조)
    toon.SpecShininess = 50.0f; // 날카로운 하이라이트
    toon.SpecThreshold = 0.55f; // 스펙큘러 컷오프
    toon.SpecMask = specMask; // LightMap R채널

    // Rim 파라미터
    const float3 rimColor = float3(1.0f, 1.0f, 1.0f); // 흰색 rim
    const float rimPower = 4.0f; // rim 두께 제어
    const float rimThreshold = 0.25f; // rim 컷오프

    // ─── Forward+ Tile 인덱스 계산 (기존과 동일) ───────────
    const uint2 pixelCoord = uint2(input.pos.xy);
    const uint tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
    const uint2 tileCoord = pixelCoord / FORWARD_PLUS_TILE_SIZE;
    const uint tileIndex = tileCoord.y * tileCountX + tileCoord.x;
    const uint2 tileMeta = ForwardPlusTileMeta[tileIndex];

    // ─── [변경] 조명 누적 루프 (Toon 버전) ─────────────────
    LightColor totalColor = (LightColor) 0.f;
    bool directionalShadowApplied = false;
    float mainShadowFactor = 1.f; // Rim Light 계산에서 재사용

    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];

        // [변경] CalculateLightColorPBR → CalculateLightColorToon
        const LightColor lc = CalculateLightColorToon(
            lightIndex, viewNormal, input.viewPos, color.rgb, toon);

        totalColor.diffuse += lc.diffuse;
        totalColor.ambient += lc.ambient;
        totalColor.specular += lc.specular;

        // ── CSM Shadow (DirectionalLight에만 적용, 기존 구조 유지) ──
        if (!directionalShadowApplied && Lights[lightIndex].lightType == 0)
        {
            const float visibility = CalculateCSMShadow(
                input.viewPos, viewNormal, Lights[lightIndex].direction.xyz);

            totalColor.diffuse *= visibility;
            totalColor.ambient *= visibility;
            // [주의] specular에도 CSM shadow 적용 (그림자 안에서 하이라이트 방지)
            totalColor.specular *= visibility;
            directionalShadowApplied = true;

            // [신규] Rim Light용 mainShadowFactor 계산
            // Directional Light의 NdotL을 기반으로 lit/shadow 면 판별
            float3 viewLightDir = normalize(mul(
                float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
            float ndotL01 = dot(-viewLightDir, normalize(viewNormal)) * 0.5f + 0.5f;
            float hs = toon.ShadowSmoothness * 0.5f;
            mainShadowFactor = smoothstep(
                toon.ShadowThreshold - hs,
                toon.ShadowThreshold + hs,
                ndotL01) * visibility;
        }
    }

    // ─── [신규] Rim Light 적용 ─────────────────────────────
    float3 rim = CalculateRimLight(
        viewNormal, input.viewPos,
        rimColor, rimPower, rimThreshold, rimMask,
        mainShadowFactor);

    // ─── [변경] 최종 색상 합성 ─────────────────────────────
    //  기존 PBR 합성:
    //    result = (diffuse × baseColor) + (ambient × baseColor) + specular
    //
    //  [변경] Toon 합성:
    //    result = diffuse + (ambient × baseColor) + specular + rim
    //
    //  이유: CalculateLightColorToon의 diffuse에 이미 baseColor가 포함됨
    //        (내부에서 litColor = baseColor × lightRGB 로 계산)
    //        ambient는 환경광 × baseColor로 별도 처리
    float3 ambientContrib = totalColor.ambient.xyz * color.xyz;
// ambient도 너무 강하면 날아가므로 saturate로 제한
    ambientContrib = saturate(ambientContrib) * 0.5f; // ambient 기여도 제한

    color.xyz = totalColor.diffuse.xyz       // Toon diffuse (baseColor 포함)
          + ambientContrib // [수정] 스케일 다운된 ambient
          + totalColor.specular.xyz;
          + rim;
    color.xyz = color.xyz / (color.xyz + 1.0f);
    return color;

}
