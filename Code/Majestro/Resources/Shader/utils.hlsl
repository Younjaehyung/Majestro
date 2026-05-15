#ifndef _UTILS_HLSL_
#define _UTILS_HLSL_


#include "params.hlsl"
#include "math.hlsl"



// Henyey-Greenstein Mie 산란 위상 함수
float MiePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159265f * pow(max(denom, 0.0001f), 1.5f));
}

// 낮은-차이 노이즈(Bayer-like dither) — 레이마칭 밴딩 억제
float BayerDither(float2 screenPos)
{
    uint2 p = uint2(screenPos) & 3u; // 4×4 타일
    uint idx = p.y * 4u + p.x;
    // 4×4 Bayer 행렬 (정규화된 0~1)
    return bayer[idx];
}

// IGN (Interleaved Gradient Noise) 노이즈
float InterleavedGradientNoise(float2 svPosition)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(svPosition, magic.xy)));
}

void ApplyIGNDitherFade(float2 svPosition, float alpha)
{
    float noise = InterleavedGradientNoise(svPosition);
    clip(alpha - noise);
}

float3 CalcWindOffset(float3 worldPos, float localY, float4 windParam)
{
    // worldPos : 월드 공간 좌표 (위상 계산에 사용)
    // localY   : 로컬 공간 Y 좌표 (뿌리 기준 높이, 0 이하면 흔들림 없음)
    // windParam: (세기, 주파수, 방향X, 방향Z)
    
    float strength = windParam.x;
    float freq = windParam.y;
    float2 dir = windParam.zw;

    if (strength <= 0.0f)
        return float3(0.0f, 0.0f, 0.0f);

    float dirLen = length(dir);
    dir = (dirLen > 0.0001f) ? (dir / dirLen) : float2(1.0f, 0.0f);


    float phase = dot(worldPos.xz, float2(0.15f, 0.23f));
    float t = PassParams.TotalTime * freq;

    // 풀포기마다 다른 타이밍 세 주파수 혼합
    float wave = sin(t + phase) * 0.60f
               + sin(t * 2.13f + phase * 1.40f) * 0.25f
               + sin(t * 0.47f + phase * 0.83f) * 0.15f;

    // 뿌리(Y <= 0)는 고정, 위로 갈수록 세게 흔들림
    float amp = max(0.0f, localY) * strength;
    float2 ofs2D = dir * wave * amp;

    return float3(ofs2D.x, 0.0f, ofs2D.y);
}



/////////////////////////////////////////////////////////////////////////////////////////
// 색수차(Chromatic Aberration) 

// UV를 NDC 기준 [-1, 1] 공간으로 변환
float2 ToNDC(float2 uv)
{
    return uv * 2.0f - 1.0f;
}

// NDC를 UV 공간으로 역변환
float2 ToUV(float2 ndc)
{
    return ndc * 0.5f + 0.5f;
}

// 화면 중심에서의 거리에 따라 오프셋 방향 벡터를 계산
// 중심에서 멀수록 색수차가 강해지는 방사형 효과
float2 CalcRadialOffset(float2 uv, float intensity)
{
    float2 ndc = ToNDC(uv); // UV -> NDC
    float dist = length(ndc); // 중심까지의 거리
    float2 dir = normalize(ndc); // 방향 벡터
    
    // dist를 제곱하면 가장자리로 갈수록 급격히 강해짐
    return dir * dist * dist * intensity;
}


/////////////////////////////////////////////////////////////////////////////////////////
// 톤매핑
float3 TonemapACES(float3 x) // ACES
{
    // Narkowicz ACES approximation
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}


float Luminance(float3 v)
{
    return dot(v, float3(0.2126f, 0.7152f, 0.0722f));
}


float3 Uncharted2Partial(float3 x)
{
    float A = 0.15f; // Shoulder Strength
    float B = 0.50f; // Linear Strength
    float C = 0.10f; // Linear Angle
    float D = 0.20f; // Toe Strength
    float E = 0.02f; // Toe Numerator
    float F = 0.30f; // Toe Denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Uncharted2Filmic(float3 color)
{
    float ExposureBias = 2.0f;
    float3 curr = Uncharted2Partial(color * ExposureBias);
    
    float W = 11.2f; // 화이트 포인트
    float3 whiteScale = 1.0f / Uncharted2Partial(float3(W, W, W));
    return curr * whiteScale;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Toon Shading


// Toon/Custom BRDF용 라이트 평가 결과
struct EvaluatedLight
{
    float3 L; // surface -> light (view space)
    float NdotL; // saturate(dot(N, L))
    float atten; // distanceRatio (spot 포함)
    float3 diffRGB; // light diffuse rgb
    float3 specRGB; // light specular rgb
    float3 ambRGB; // light ambient rgb
    int type; // 0 dir / 1 point / else spot
};

EvaluatedLight EvaluateLightVS(int lightIndex, float3 viewPos, float3 viewNormal)
{
    EvaluatedLight o = (EvaluatedLight) 0;

    o.type = Lights[lightIndex].lightType;
    o.diffRGB = Lights[lightIndex].color.diffuse.rgb;
    o.specRGB = Lights[lightIndex].color.specular.rgb;
    o.ambRGB = Lights[lightIndex].color.ambient.rgb;

    float3 viewLightDir = 0.0f; // light -> surface
    float distanceRatio = 1.0f;

    if (o.type == 0)
    {
        // Directional
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        o.L = normalize(-viewLightDir); // surface -> light (view space)
        o.NdotL = saturate(dot(o.L, viewNormal));
        o.atten = 1.0f;
    }
    else if (o.type == 1)
    {
        // Point
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos); // light -> surface
        o.L = normalize(-viewLightDir);
        o.NdotL = saturate(dot(o.L, viewNormal));

        float dist = distance(viewPos, viewLightPos);
        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
            distanceRatio = saturate(1.f - pow(dist / Lights[lightIndex].range, 2));

        o.atten = distanceRatio;
    }
    else
    {
        // Spot
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos); // light -> surface
        o.L = normalize(-viewLightDir);
        o.NdotL = saturate(dot(o.L, viewNormal));

        if (Lights[lightIndex].range == 0.f)
        {
            o.atten = 0.f;
        }
        else
        {
            float halfAngle = Lights[lightIndex].angle * 0.5f;

            float3 viewLightVec = viewPos - viewLightPos;
            float3 viewCenterLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);

            float centerDist = dot(viewLightVec, viewCenterLightDir);
            float lightAngle = acos(dot(normalize(viewLightVec), viewCenterLightDir));

            if (centerDist < 0.f || centerDist > Lights[lightIndex].range)
                distanceRatio = 0.f;
            else if (lightAngle > halfAngle)
                distanceRatio = 0.f;
            else
                distanceRatio = saturate(1.f - pow(centerDist / Lights[lightIndex].range, 2));

            o.atten = distanceRatio;
        }
    }

    return o;
}
// 전신용 Toon 파라미터(임시 상수)
static const float gToonShadowEdge = 0.50f; // 그림자 경계
static const float gToonShadowSoft = 0.08f; // 경계 부드러움
static const float3 gToonShadowTint = float3(0.60f, 0.65f, 0.75f); // 그림자 틴트

static const float gSpecPower = 48.0f; // 스펙 날카로움
static const float gSpecEdge = 0.25f; // 밴드 경계
static const float gSpecSoft = 0.10f; // 밴드 소프트
static const float gSpecIntensity = 1.00f; // 스펙 강도

static const float gOtherDiffScale = 0.35f; // 보조 라이트 diffuse 약화
static const float gOtherSpecScale = 0.25f; // 보조 라이트 spec 약화

float ToonShadowStep(float NdotL)
{
    // [추가] step 대신 smoothstep으로 계단 경계를 안정화
    return smoothstep(gToonShadowEdge - gToonShadowSoft, gToonShadowEdge + gToonShadowSoft, NdotL);
}

float3 ToonDiffuse(float3 baseColor, float3 lightRGB, float atten, float step01)
{
    float3 lit = (lightRGB * atten) * baseColor;
    float3 shd = (lightRGB * atten) * (baseColor * gToonShadowTint);
    return lerp(shd, lit, step01);
}

float ToonSpecBand(float NdotH)
{
    float s = pow(saturate(NdotH), gSpecPower);
    return smoothstep(gSpecEdge - gSpecSoft, gSpecEdge + gSpecSoft, s);
}


// ============================================================
//  CalculateRimLight
//
//  [기법] Depth-based Screen-Space Rim Light (원신 스타일)
//
//  Fresnel 기반의 문제점:
//   - NdotV로 계산하면 실루엣 외에 몸체 내부 굴곡(팔뚝 옆면 등)에도 rim이 발생
//
//  원신 방식:
//   - View Normal의 스크린 투영 방향으로 N픽셀 오프셋
//   - 오프셋 위치의 Depth를 Gbuffer[0]에서 샘플링
//   - 현재 픽셀 Depth와 차이가 크다 = 뒤에 다른 오브젝트/배경 = 실루엣 엣지
//   - 실루엣 엣지에만 rim 적용 → 몸체 내부 굴곡 제외
//
//  [파라미터]
//   screenPos    : input.pos.xy (현재 픽셀 화면 좌표)
//   viewNormal   : View Space Normal (normalize된)
//   rimColor     : rim 색상
//   rimWidth     : 오프셋 픽셀 수 (권장 1~3)
//   rimDepthThres: depth 차이 임계값 (권장 0.005~0.02)
//   rimMask      : LightMap A채널 마스크
//   shadowFactor : 0 = 그림자 면, 1 = 밝은 면
//                  그림자 면에서 rim을 더 강조 (역광 연출)
// ============================================================
float3 CalculateRimDepthLight(
    float2 screenPos,
    float3 viewNormal,
    float3 rimColor,
    float rimWidth,
    float rimDepthThres,
    float rimMask,
    float shadowFactor)
{
    // View Normal XY를 스크린 공간 오프셋 방향으로 근사
    // View Space Normal.xy는 카메라 기준 좌우/상하 방향 -> 스크린 방향과 근사 일치
    float2 normalSS = normalize(viewNormal.xy + 1e-5f);
    int2 offset = int2(normalSS * rimWidth);

    int3 curCoord = int3((int2) screenPos, 0);
    int3 offCoord = int3((int2) screenPos + offset, 0);

    // Gbuffer[0] = PRE_DEPTH (R32_FLOAT SRV)
    // R32_FLOAT SRV를 float4로 읽으면 .r = depth, .gba = 0/1
    float curDepth = Gbuffer[0].Load(curCoord).r;
    float offDepth = Gbuffer[0].Load(offCoord).r;

    // 오프셋 위치가 현재보다 카메라에 가깝다 (더 작은 depth)
    // = 실루엣 엣지 (뒤에 다른 오브젝트/배경이 있음)
    float depthDiff = curDepth - offDepth;
    float rim = step(rimDepthThres, depthDiff);

    // 그림자 면(shadowFactor=0)에서 rim 더 강하게 (역광 연출)
    float rimBoost = lerp(1.2f, 0.5f, shadowFactor);
    rim *= rimBoost * rimMask;

    return rimColor * saturate(rim);
}

//float3 CalculateRimLightSS(
//    float2 screenPos, // 수정: 반드시 픽셀 좌표 기준이어야 함
//    float3 viewNormal,
//    float3 viewDir,
//    float3 mainLightDirVS,
//    float viewDepth, // 수정: linear view depth라고 가정
//    float3 rimColor,
//    float rimWidth,
//    float rimFeather,
//    float rimIntensity,
//    float rimMask,
//    float shadowFactor)
//{
//    float3 N = normalize(viewNormal);
//    float3 V = normalize(viewDir);

//    // 수정: 2D 광원 방향이 너무 작을 때의 불안정한 normalize 방지
//    float2 L_View = mainLightDirVS.xy;
//    float lenL = length(L_View);
//    L_View = (lenL > 1e-5f) ? (L_View / lenL) : float2(0.0f, 0.0f);

//    // 수정: 화면 평면 노멀도 동일하게 안전 normalize
//    float2 N_View = N.xy;
//    float lenN = length(N_View);
//    N_View = (lenN > 1e-5f) ? (N_View / lenN) : float2(0.0f, 0.0f);

//    // 수정: 조명 방향 기반 보정
//    float lDotN = saturate(dot(N_View, L_View));

//    // 수정: int 캐스팅 대신 round 사용해서 계단 현상 완화
//    int2 offset = int2(round(N_View * (rimWidth * lDotN)));

//    int2 pixelPos = int2(screenPos);

//    int2 maxPix = int2(PassParams.ScreenSize) - int2(1, 1);
//    int2 curPix = clamp(pixelPos, int2(0, 0), maxPix);
//    int2 offPix = clamp(pixelPos + offset, int2(0, 0), maxPix);

//    int3 curCoord = int3(curPix, 0);
//    int3 offCoord = int3(offPix, 0);

//    // 중요:
//    // 아래 depth는 반드시 linear depth 여야 함.
//    // 만약 Gbuffer[0]이 device depth라면 linearize 후 비교해야 함.
//    float curDepth = Gbuffer[0].Load(curCoord).r;
//    float offDepth = Gbuffer[0].Load(offCoord).r;

//    // 수정:
//    // 엔진의 depth 정의에 따라 부호가 반대일 수 있음.
//    // 일반적으로 "오프셋 쪽이 더 멀다"를 edge로 보려면 off - cur를 먼저 의심하는 것이 보통 더 자연스러움.
//    float depthDiff = offDepth - curDepth;

//    // 수정:
//    // threshold와 feather를 분리해서 사용
//    // viewDepth 기반 스케일을 주되, 최소값 보장
//    float depthScale = max(viewDepth, 1e-3f);

//    float threshold = rimWidth * 0.0025f * depthScale;
//    float feather = max(rimFeather * 0.0025f * depthScale, 1e-5f);

//    float edge = smoothstep(threshold - feather, threshold + feather, depthDiff);

//    // Fresnel 보조
//    float f = 1.0f - saturate(dot(V, N));
//    float fresnelBoost = smoothstep(0.2f, 1.0f, f);

//    // 이 부분은 아트 의도에 따라 유지 가능
//    float shadowMul = lerp(1.2f, 0.6f, saturate(shadowFactor));

//    float intensity = edge * fresnelBoost * shadowMul * rimIntensity * rimMask;
//    return rimColor * saturate(intensity);
//}

float3 CalculateRimLightSS(
    float2 screenPos,
    float3 viewNormal,
    float3 viewDir,
    float3 mainLightDirVS,
    float viewDepth,
    float3 rimColor,
    float rimWidth,
    float rimFeather,
    float rimIntensity,
    float rimMask,
    float shadowFactor)
{
    float3 N = normalize(viewNormal);
    float3 V = normalize(viewDir);


    float2 L_View = normalize(mainLightDirVS.xy + 1e-5f);
    float2 N_View = normalize(N.xy + 1e-5f);

    // NdotL 기반 길이 보정
    float lDotN = saturate(dot(N_View, L_View) + rimWidth * 0.1f);

    // 스크린 좌표 오프셋
    int2 offset = int2(N_View * lDotN * rimWidth);
    int2 pixelPos = int2(screenPos);

    // 안전 clamp (화면 밖 접근 방지)
    int2 maxPix = int2(PassParams.ScreenSize) - int2(1, 1);
    int2 curPix = clamp(pixelPos, int2(0, 0), maxPix);
    int2 offPix = clamp(pixelPos + offset, int2(0, 0), maxPix);

    int3 curCoord = int3(curPix, 0);
    int3 offCoord = int3(offPix, 0);

    // PRE_DEPTH 로드
    float curDepth = Gbuffer[0].Load(curCoord).r;
    float offDepth = Gbuffer[0].Load(offCoord).r;
    float depthDiff = curDepth - offDepth;

    // depth 기반 feather
    float depthScale = max(viewDepth, 1e-3f);
    float edge0 = 0.24f * rimFeather * depthScale;
    float edge1 = 0.25f * depthScale;
    float edge = smoothstep(edge0, edge1, depthDiff);

    // Fresnel 보조 (실루엣 강조)
    float f = 1.0f - saturate(dot(V, N));
    float fresnelBoost = smoothstep(0.2f, 1.0f, f);

    // 그림자/마스크/강도
    float shadowMul = lerp(1.2f, 0.6f, saturate(shadowFactor));
    float intensity = edge * fresnelBoost * shadowMul * rimIntensity * rimMask;

    return rimColor * saturate(intensity);
}

float3 NPR_Base_RimLight(float NdotV, float NdotL, float3 baseColor)
{
    return (1 - smoothstep(0.5f, 0.5f + 0.03, NdotV)) * 0.5f * (1 - (NdotL * 0.5 + 0.5)) * baseColor;
}

float3 CalculateFresnelRimLight(
    float3 viewPos, float3 viewNormal, float3 rimColor, float rimPower, float rimMask)
{
    float3 V = normalize(-viewPos);
    float3 N = normalize(viewNormal);

   // fresnel 기반 rim 입력
    float f = 1.0f - saturate(dot(V, N));


    float rimMin = 0.75f;
    float rimMax = 1.f;
    float rimSmooth = 0.30f;
    float3 rimColorRGB = float3(1.0f, 1.0f, 1.0f);
    float rimStrength = rimMask; // 또는 1.0f, 혹은 rimMask * 0.8f

    float rim = smoothstep(rimMin, rimMax, f);
    rim = smoothstep(0.0f, rimSmooth, rim);

    rimColor = rim * rimColorRGB * rimStrength;
    return rimColor;
}

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

    // Ramp Texture
    // >= 0 : TextureMaps[RampTexIdx]를 1D RampTexutre
    //         U = Half-Lambert NdotL [0=그림자, 1=밝은면]
    //         V = 0.5 (1D lookup)
    //         R 채널 = shadowFactor (0=shadColor, 1=litColor)
    //  -1   : ShadowThreshold/ShadowSmoothness 기반 smoothstep

    int RampTexIdx;
};


struct ToonSpecParams
{
    float3 SpecColor; // lerp(0.04, baseColor, metallic)
    float Glossiness; // 1 - roughness
    float Shininess; // glossiness에서 유도한 pow 지수
    float Threshold; // glossiness 또는 LightMap.b에서 유도한 컷오프
};

ToonSpecParams CalcSpecParams(float3 baseColor, float metallic, float roughness, float lightMapB)
{
    ToonSpecParams p;
    p.SpecColor = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    p.Glossiness = 1.0f - roughness;
    p.Shininess = exp2(p.Glossiness * 10.0f + 1.0f); // 2 ~ 2048 범위
    p.Threshold = 1.0f - saturate(lightMapB); // LightMap.b 기반 컷오프
    return p;
}

float3 CalculateToonSpecular1(float NdotL, float NdotH, float3 normalDir, float3 baseColor, float4 parameter)
{
    float Ks = 0.04;
    float SpecularPow = exp2(0.5 * parameter.r * 11.0 + 2.0);
    float SpecularNorm = (SpecularPow + 8.0) / 8.0;
    float3 SpecularColor = baseColor * parameter.g;
    // [수정] baseColor(float3) 제거 — SpecularContrib은 스칼라여야 함
    float SpecularContrib = SpecularNorm * pow(saturate(NdotH), SpecularPow);

    // [수정] UNITY_MATRIX_V → PassParams.MatView, float3 변환 후 .x 추출
    float3 viewNormal = normalize(mul(float4(normalDir, 0.f), PassParams.MatView).xyz);
    float MetalDir = viewNormal.x;
    float MetalRadius = saturate(1.0 - MetalDir) * saturate(1.0 + MetalDir);
    float MetalFactor = saturate(step(0.5, MetalRadius) + 0.25) * 0.5 * saturate(step(0.15, MetalRadius) + 0.25) * lerp(0.1f * 5, 0.8f * 10, parameter.b);

    float3 MetalColor = MetalFactor * baseColor * step(0.95, parameter.r);
    return SpecularColor * (SpecularContrib * NdotL * Ks * parameter.b + MetalColor);
}

float3 CalculateToonSpecular2(
    float NdotL,
    float NdotH,
    float3 viewNormal,
    float3 baseColor,
    float metallicMask,
    float roughnessMask,
    float responseMask,
    float metalIntensity)
{
    const float3 DielectricF0 = float3(0.04f, 0.04f, 0.04f);

   
    float specularPow = lerp(128.0f, 8.0f, saturate(roughnessMask));
    float specularNorm = (specularPow + 8.0f) / 8.0f;

    //  metallic 기반으로 spec color 계산
    float3 specularColor = lerp(DielectricF0, baseColor, saturate(metallicMask));

    float rawSpec = specularNorm * pow(saturate(NdotH), specularPow);


    float specularContrib = smoothstep(0.4f, 0.5f, rawSpec);

   
    float2 m = normalize(viewNormal.xy + 1e-5f);
    float2 metalRadius2 = saturate(1.0f - m) * saturate(1.0f + m);
    float metalRadius = saturate(max(metalRadius2.x, metalRadius2.y));

    float metalFactor =
        saturate(step(0.5f, metalRadius) + 0.25f) *
        0.5f *
        saturate(step(0.15f, metalRadius) + 0.25f) *
        lerp(metalIntensity * 0.5f, metalIntensity, saturate(responseMask));

    float metalMask = smoothstep(0.7f, 0.95f, metallicMask);
    float3 metalColor = metalFactor * baseColor * metalMask;

    float lightMask = smoothstep(0.0f, 0.1f, saturate(NdotL));

    return specularColor * (specularContrib * lightMask * saturate(responseMask))
         + metalColor;
}


float3 CalculateToonSpecular3(
    float NdotL,
    float NdotH,
    float3 viewNormal,
    float3 baseColor,
    float metallicMask,
    float roughnessMask,
    float responseMask,
    float metalIntensity,
    float specThreshold, // 비금속 spec의 임계값 (낮을수록 하이라이트 넓어짐)
    float specSoftness)  // 비금속 spec의 경계 부드러움
{

    float metalMask = smoothstep(0.45f, 0.75f, saturate(metallicMask));
    float nonMetalMask = 1.0f - metalMask;

    // 빛이 닿는 면에서만 spec 허용
    float lightMask = smoothstep(0.0f, 0.1f, saturate(NdotL)) * saturate(responseMask);


    // 비금속 Specular
    //   glossiness가 없어서 roughness 기반으로 대체했음
    float glossiness = 1.0f - saturate(roughnessMask); // 0(거칠)~1(매끈)
    float specularPow = exp2(glossiness * 10.0f + 1.0f); // ~2 ~ 2048

    float rawSpec = pow(saturate(NdotH), specularPow);
    float toonSpec = smoothstep(specThreshold - specSoftness,
                                specThreshold + specSoftness,
                                rawSpec);

    float specIntensity = lerp(0.1f, 1.0f, glossiness);
    float3 nonMetalSpec = float3(specIntensity, specIntensity, specIntensity)
                         * toonSpec * lightMask * nonMetalMask;


    // 금속 Specular
    // (MatCap 방식)
    float metalRadius = viewNormal.z * viewNormal.z;
    float bandWidth = lerp(0.30f, 0.12f, glossiness);
    float bandCenter = 0.50f;
    float bandInner = smoothstep(bandCenter - bandWidth, bandCenter, metalRadius);
    float bandOuter = 1.0f - smoothstep(bandCenter, bandCenter + bandWidth, metalRadius);
    float metalBand = bandInner * bandOuter;

    // 금속 하이라이트 색상
    float3 metalHighlight = lerp(baseColor, baseColor * 2.5f + 0.3f, glossiness * 0.4f);

    // lightMask를 약하게 적용
    float metalLightFade = lerp(0.35f, 1.0f, lightMask);
    float3 metalSpec = metalHighlight * metalBand * metalIntensity * metalLightFade * metalMask;

    // 비금속 하이라이트 + 금속 MatCap 띠
    return nonMetalSpec + metalSpec;
}


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

    if (distanceRatio <= 0.f)
        return color;

    
    float shadowFactor = 0.0f;

    if (toon.RampTexIdx >= 0)
    {
        float halfLambert = rawNdotL * 0.5f + 0.5f;
        shadowFactor = TextureMaps[toon.RampTexIdx]
        .Sample(g_sam_Terrain, float2(halfLambert, 0.5f)).r;
    }
    else
    {
        float halfSmooth = toon.ShadowSmoothness * 0.5f;
        float ndotL01 = saturate(rawNdotL);
        shadowFactor = smoothstep(
        toon.ShadowThreshold - halfSmooth,
        toon.ShadowThreshold + halfSmooth,
        ndotL01);
    }

   
    float3 shadColor = float3(224.f / 255.f, 187.f / 255.f, 178.f / 255.f);
    float3 litColor = float3(1.f, 1.f, 1.f);
    float3 lerpColor = lerp(shadColor, litColor, shadowFactor);
    float4 colorLight = Lights[lightIndex].color.ambient * distanceRatio;

    color.diffuse = float4(baseColor * lerpColor, 1.0f);
    color.specular = float4(0, 0, 0, 1);


    return color;
}



/////////////////////////////////////////////////////////////////////////////////////////
// PBR


float3 FresnelSchlick(float cosTheta, float3 F0)
{
    // Schlick 근사
    // F : 절연체는 낮은 값 
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float NdotH, float roughness)
{
    //GGX / Trowbridge-Reitz NDF
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom + 1e-6f);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{

    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotX / (NdotX * (1.0f - k) + k + 1e-6f);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    // Smith geometry (separable)
    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

// Burley 이방성 GGX — 실크/스타킹 (tangent 축으로 스페큘러 늘임)
float DistributionGGX_Anisotropic(float3 H, float3 T, float3 B, float3 N,
                                  float roughX, float roughY)
{
    float ax = max(roughX * roughX, 1e-4f);
    float ay = max(roughY * roughY, 1e-4f);
    float TdotH = dot(T, H);
    float BdotH = dot(B, H);
    float NdotH = dot(N, H);
    float denom = (TdotH * TdotH) / (ax * ax)
                + (BdotH * BdotH) / (ay * ay)
                + NdotH * NdotH;
    return 1.0f / (PI * ax * ay * denom * denom + 1e-6f);
}

LightColor CalculateLightColorPBR(int lightIndex, float3 viewNormal, float3 viewPos,
                                  float3 baseColor, float metallic, float roughness)
{
    LightColor color = (LightColor) 0.f;

    float3 viewLightDir = (float3) 0.f;
    float distanceRatio = 1.f;
    float NdotL = 0.f;

    if (Lights[lightIndex].lightType == 0)
    {
        // Directional
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        NdotL = saturate(dot(-viewLightDir, viewNormal));
    }
    else if (Lights[lightIndex].lightType == 1)
    {
        // Point
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        NdotL = saturate(dot(-viewLightDir, viewNormal));

        float dist = distance(viewPos, viewLightPos);
        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
            distanceRatio = saturate(1.f - pow(dist / Lights[lightIndex].range, 2));
    }
    else
    {

        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        NdotL = saturate(dot(-viewLightDir, viewNormal));

        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
        {
            float halfAngle = Lights[lightIndex].angle / 2;

            float3 viewLightVec = viewPos - viewLightPos;
            float3 viewCenterLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);

            float centerDist = dot(viewLightVec, viewCenterLightDir);
            float lightAngle = acos(dot(normalize(viewLightVec), viewCenterLightDir));

            if (centerDist < 0.f || centerDist > Lights[lightIndex].range)
                distanceRatio = 0.f;
            else if (lightAngle > halfAngle)
                distanceRatio = 0.f;
            else
                distanceRatio = saturate(1.f - pow(centerDist / Lights[lightIndex].range, 2));
        }
    }


    if (NdotL <= 0.0f || distanceRatio <= 0.0f)
    {
        color.ambient = Lights[lightIndex].color.ambient * float4(baseColor, 1.f) * distanceRatio;
        return color;
    }

    // -----------------------------
    // 2) PBR BRDF (Cook-Torrance GGX)
    // -----------------------------
    float3 N = normalize(viewNormal);

    float3 V = normalize(-viewPos);

  
    float3 L = normalize(-viewLightDir);

    float3 H = normalize(V + L);

    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

 
    roughness = max(roughness, 0.04f);

  
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);

    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float3 F = FresnelSchlick(VdotH, F0);

    float3 numerator = D * G * F;
    float denom = max(4.0f * NdotV * NdotL, 1e-6f);
    float3 specular = numerator / denom;

   
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 diffuse = kD;

    // -----------------------------
    // 3) 라이트 색/세기 적용
    // -----------------------------
    float3 radiance = Lights[lightIndex].color.diffuse.rgb * distanceRatio;

    float3 outDiffuse = diffuse * radiance * NdotL;
    float3 outSpecular = specular * radiance * NdotL;

    color.diffuse = float4(outDiffuse, 1.0f);
    color.specular = float4(outSpecular, 1.0f);

  
    color.ambient = Lights[lightIndex].color.ambient * float4(baseColor, 1.f) * distanceRatio;

    return color;
}

/////////////////////////////////////////////////////////////////////////////////////////
// PBR+NPR

/////////////////////////////////////////////////////////////////////////////////////////
// PBR+NPR

// halfLambert을 smoothstep (Sigmoid)
float NPR_Sigmoid(float x, float offset, float smooth)
{
    return smoothstep(offset - smooth, offset + smooth, x);
}

// Ramp 텍스쳐

float3 SampleShadowRamp(int texIdx, float ndotLRemap)
{ // (V=0.25): 난반사 그림자 색상
    return TextureMaps[texIdx].Sample(g_sam_Terrain, float2(ndotLRemap, 0.25f)).rgb;
}

float3 SampleSpecularRamp(int texIdx, float specRange)
{ // (V=0.75): 정반사 스타일 색상
    return TextureMaps[texIdx].Sample(g_sam_Terrain, float2(specRange, 0.75f)).rgb;
}

struct PBRNPRShadingParams
{
    float ShadowOffset; // Sigmoid 명암 경계 (0.5 기본, shadowBias 포함)
    float ShadowSmooth; // Sigmoid 경계 폭   (0.08 기본)
    float ShadowStrength; // 그림자 전체 강도  (1.0 기본)
    float3 ShadowColor; // 1차 그림자 틴트   (청회색 계열)
    float3 SecShadowColor; // 2차 그림자 틴트   (ILM AO 기반 섞음)
    float SpecMask; // ILM Spec 마스크   (lightMap.r)
    float ilmAO; // ILM AO            (lightMap.g, 0.5=중립)
    float IBLScale; // 환경광 약화 스케일 (0.15 기본)
    int RampTexIdx; // Shadow Ramp 텍스처
    uint MatClass; // 재질 분류 (ClassifyMaterial 결과)
    float3 ViewTangent; // 이방성 GGX용 (MAT_SILK)
    float3 ViewBinormal; // 이방성 GGX용 (MAT_SILK)
};

LightColor CalculateLightColorPBRNPR1(
    int lightIndex,
    float3 viewNormal,
    float3 viewPos,
    float3 albedo,
    float metallic,
    float roughness,
    PBRNPRShadingParams npr)
{
    LightColor result = (LightColor) 0;

    float3 viewLightDir = 0.0f;
    float distanceRatio = 1.0f;
    float rawNdotL = 0.0f;

    if (Lights[lightIndex].lightType == 0)
    {
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        rawNdotL = dot(-viewLightDir, viewNormal);
    }
    else if (Lights[lightIndex].lightType == 1)
    {
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
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        rawNdotL = dot(-viewLightDir, viewNormal);
        if (Lights[lightIndex].range == 0.f)
        {
            distanceRatio = 0.f;
        }
        else
        {
            float halfAngle = Lights[lightIndex].angle * 0.5f;
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

    if (distanceRatio <= 0.f)
        return result;

    float3 N = normalize(viewNormal);
    float3 V = normalize(-viewPos);
    float3 L = normalize(-viewLightDir);
    float3 H = normalize(V + L);

    float NdotV = max(saturate(dot(N, V)), 0.001f);
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    // NPR 난반사: Sigmoid
    float halfLambert = rawNdotL * 0.5f + 0.5f;
    float shadowArea = NPR_Sigmoid(1.0f - halfLambert, npr.ShadowOffset, npr.ShadowSmooth)
                      * npr.ShadowStrength;
    float NdotLRemap = 1.0f - shadowArea; // 0=그림자

    // Ramp 색상 샘플링
    float3 shadowRamp;
    if (npr.RampTexIdx >= 0)
        shadowRamp = SampleShadowRamp(npr.RampTexIdx, NdotLRemap);
    else
        shadowRamp = lerp(npr.ShadowColor, float3(1.0f, 1.0f, 1.0f), NdotLRemap);

    // AO 2차 그림자색
    shadowRamp = lerp(npr.SecShadowColor, shadowRamp, saturate(npr.ilmAO + 0.5f));

    // Fresnel
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 F = FresnelSchlick(HdotV, F0);

    // NPR용 kDiff: diffuse 밝기 상향 보정했음
    float3 kDiff = ((1.0f - F) * 0.5f + 0.5f) * (1.0f - metallic);
    float3 directDiffColor = kDiff * albedo;

    // GGX 정반사
    roughness = max(roughness, 0.04f);
    float NdotLClamped = max(NdotLRemap, 0.001f);
    // ILM SpecMask을 NDF 마스킹으로 사용
    float NDF = DistributionGGX(NdotH, roughness) * npr.SpecMask;
    float G = GeometrySmith(NdotV, NdotLClamped, roughness);
    float3 nom = NDF * G * F;
    float denom = 4.0f * NdotV * NdotLClamped + 0.0001f;
    float3 BRDFSpec = nom / denom;

    float3 directSpecColor;
    if (npr.RampTexIdx >= 0)
    {
        // Ramp로 정반사 색상 샘플링
        float specRange = saturate(NDF * G / max(denom, 0.0001f));
        float3 specRampCol = SampleSpecularRamp(npr.RampTexIdx, specRange);
        directSpecColor = specRampCol * F;
    }
    else
    {
        directSpecColor = BRDFSpec * PI;
    }

    // final color
    float3 lightColor = Lights[lightIndex].color.diffuse.rgb * distanceRatio;
    float3 directLight = (directDiffColor * shadowRamp + directSpecColor * NdotLRemap)
                       * lightColor;

    result.diffuse = float4(directLight, 1.0f);
    return result;
}


// ============================================================
// Material Class — ILM LightMap.b 5단계 흑백 마스크
//   0.0 단단한 물체         Hard Default
//   0.2 부드러운 물체       Soft Matte 
//   0.4 메탈/메탈 프로젝션  Metal              (toon ramp 우회, PBR 통과)
//   0.6 실크/스타킹         Silk / Stocking    (이방성 스페큘러)
//   0.8 머리카락            Hair
//   1.0 피부                Skin               (Wrap lighting + SSS 근사)
// ============================================================
#define MAT_HARD   0
#define MAT_SOFT   1
#define MAT_METAL  2
#define MAT_SILK   3
#define MAT_HAIR   4
#define MAT_SKIN   5


uint ClassifyMaterial(float b)
{
    float v = saturate(b) * 10.0f;
    if (v < 1.0f)
        return MAT_HARD;
    if (v < 3.0f)
        return MAT_SOFT;
    if (v < 5.0f)
        return MAT_METAL;
    if (v < 7.0f)
        return MAT_SILK;
    if (v < 9.0f)
        return MAT_HAIR;
    return MAT_SKIN;
}



LightColor CalculateLightColorPBRNPR(
    int lightIndex,
    float3 viewNormal,
    float3 viewPos,
    float3 albedo,
    float metallic,
    float roughness,
    PBRNPRShadingParams npr)
{
    LightColor result = (LightColor) 0;

    float3 viewLightDir = 0.0f;
    float distanceRatio = 1.0f;
    float rawNdotL = 0.0f;

    if (Lights[lightIndex].lightType == 0)
    {
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        rawNdotL = dot(-viewLightDir, viewNormal);
    }
    else if (Lights[lightIndex].lightType == 1)
    {
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
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        rawNdotL = dot(-viewLightDir, viewNormal);
        if (Lights[lightIndex].range == 0.f)
        {
            distanceRatio = 0.f;
        }
        else
        {
            float halfAngle = Lights[lightIndex].angle * 0.5f;
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

    if (distanceRatio <= 0.f)
        return result;

    float3 N = normalize(viewNormal);
    float3 V = normalize(-viewPos);
    float3 L = normalize(-viewLightDir);
    float3 H = normalize(V + L);

    float NdotV = max(saturate(dot(N, V)), 0.001f);
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    float halfLambert = rawNdotL * 0.5f + 0.5f;


    // 재질별 Shadow 영역
    float shadowArea;
    [branch]
    switch (npr.MatClass)
    {
        case MAT_SKIN:
        {
            // Skin: Wrap lighting — 경계 완만, 그늘까지 빛이 들어옴
                const float wrap = 0.5f;
                float wrapped = saturate((rawNdotL + wrap) / (1.0f + wrap));
                shadowArea = 1.0f - wrapped;
                break;
        }
        case MAT_HAIR:
        {
            // Hair: Wrap lighting — 경계 완만, 그늘까지 빛이 들어옴
                const float wrap = 0.5f;
                float wrapped = saturate((rawNdotL + wrap) / (1.0f + wrap));
                shadowArea = 1.0f - wrapped;
                break;
        }
        case MAT_SILK:
        {
            // Silk, Stocking : 기본 Sigmoid보다 1.5배 부드럽게
                shadowArea = NPR_Sigmoid(1.0f - halfLambert,
                                     npr.ShadowOffset,
                                     npr.ShadowSmooth * 1.5f);
                break;
        }
        case MAT_METAL:
        {
            // Metal: Sigmoid 경계를 기본보다 좁게 
            
            shadowArea = NPR_Sigmoid(1.0f - halfLambert,
                                     npr.ShadowOffset,
                                     npr.ShadowSmooth * 0.6f);
            break;
        }
        case MAT_SOFT:
        {
            // Soft Matte : 넓은 반음영
                shadowArea = NPR_Sigmoid(1.0f - halfLambert,
                                     npr.ShadowOffset,
                                     npr.ShadowSmooth * 2.0f);
                break;
        }
        default: // Hard Default
        {
                shadowArea = NPR_Sigmoid(1.0f - halfLambert, npr.ShadowOffset, npr.ShadowSmooth);
                break;
        }
    }
    shadowArea *= npr.ShadowStrength;
    float NdotLRemap = saturate(1.0f - shadowArea); // 0=그림자, 1=빛

    // Shadow Ramp 샘플링
    float3 shadowRamp;
    if (npr.RampTexIdx >= 0)
        shadowRamp = SampleShadowRamp(npr.RampTexIdx, NdotLRemap);
    else
        shadowRamp = lerp(npr.ShadowColor, float3(1.0f, 1.0f, 1.0f), NdotLRemap);

    // 2차 그림자색 (피부/메탈은 덜 섞어 원색 유지)
    float aoBlend = (npr.MatClass == MAT_METAL || npr.MatClass == MAT_SKIN)
                  ? saturate(npr.ilmAO + 0.8f) : saturate(npr.ilmAO + 0.5f);
    shadowRamp = lerp(npr.SecShadowColor, shadowRamp, aoBlend);

    
    
    //  재질별 물성 보정
    float  effectiveMetallic = metallic;
    float3 F0;
    switch (npr.MatClass)
    {
        case MAT_METAL:
           // effectiveMetallic = 1.0f;              // 메탈은 항상 풀 메탈로 처리
           //F0 = albedo;                           // 풀 색 반사
            effectiveMetallic = max(metallic, 0.75f);
            F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, effectiveMetallic);
            break;
        case MAT_SKIN:
            F0 = float3(0.028f, 0.028f, 0.028f);   // 피부: 낮게
            break;
        case MAT_SILK:
            F0 = lerp(float3(0.05f, 0.05f, 0.05f), albedo, metallic);
            break;
        default:
            F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
            break;
    }
    float3 F = FresnelSchlick(HdotV, F0);
    float oneMinusReflectivity = 1.0f - saturate(effectiveMetallic);


    // Diffuse 색 — PBR 공식 기반 (kDiff = (1-F)*(1-metallic))
  
    float3 directDiffColor;
    if (npr.MatClass == MAT_SKIN)
    {
        // 피부:PBR에서 벗어난 NPR 표현
        float3 kDiff = (1.0f - F) * oneMinusReflectivity;
        float  sssBand = saturate(1.0f - abs(rawNdotL * 2.0f - 0.5f));
        float3 sssTint = float3(0.9f, 0.4f, 0.35f);
        directDiffColor = kDiff * albedo + sssBand * sssTint * albedo * 0.12f;
    }
    //else if (npr.MatClass == MAT_METAL)
    //{  // 메탈은 effectiveMetallic=1이므로 자연스럽게 0에 수렴
    //    float metalFill = lerp(0.10f, 0.32f, NdotLRemap);
    //    float metalReflectivityReduce = lerp(1.0f, 0.35f, saturate(effectiveMetallic));
    //    directDiffColor = albedo * metalFill * metalReflectivityReduce;
    //}
    else
    {
        // 표준 NPR diffuse — (1-F) 보정 + metallic에 따라 감쇠
        // 밝기 상향(0.5 오프셋)은 NPR 스타일
        float3 kDiff = ((1.0f - F) * 0.5f + 0.5f) * oneMinusReflectivity;
        directDiffColor = kDiff * albedo;
    }


    // 재질별 Specular NDF

    roughness = max(roughness, 0.04f);
    float NdotLClamped = max(NdotLRemap, 0.001f);

    float NDF;
    float G = GeometrySmith(NdotV, NdotLClamped, roughness);

    if (npr.MatClass == MAT_SILK)
    {
        // 이방성 GGX — tangent 축은 날카롭게, binormal 축은 늘어짐
        NDF = DistributionGGX_Anisotropic(H,
                                          npr.ViewTangent,
                                          npr.ViewBinormal,
                                          N,
                                          roughness * 0.3f,
                                          roughness) * npr.SpecMask;
    }
    else if (npr.MatClass == MAT_SKIN)
    {
        // 피부는 specular 크게 억제
        NDF = DistributionGGX(NdotH, roughness) * npr.SpecMask * 0.15f;
    }
    else
    {
        NDF = DistributionGGX(NdotH, roughness) * npr.SpecMask;
    }

    float denom = 4.0f * NdotV * NdotLClamped + 0.0001f;
    float3 BRDFSpec = (NDF * G * F) / denom;


    // Specular Ramp —  BRDF는 PBR로 계산되고, 그 결과만 Ramp로 계단화
    float3 directSpecColor;
    if (npr.RampTexIdx >= 0)
    {
        float  specRange   = saturate(NDF * G / max(denom, 0.0001f));
        float3 specRampCol = SampleSpecularRamp(npr.RampTexIdx, specRange);

        float specBoost = saturate(dot(BRDFSpec, float3(0.299f, 0.587f, 0.114f)));
        float specBoostScale = (npr.MatClass == MAT_METAL) ? 0.65f : 0.35f;
        directSpecColor = F * clamp(specRampCol + specBoost * specBoostScale, 0.0f, 10.0f);
    }
    else
    {
        directSpecColor = BRDFSpec * PI;
    }

    // Final Color 계산
    float specFloor;
    switch (npr.MatClass)
    {
        case MAT_METAL: specFloor = 0.25f; break;
        case MAT_SILK:  specFloor = 0.08f; break;
        case MAT_HAIR:  specFloor = 0.10f; break;
        default:        specFloor = 0.0f;  break;
    }
    float specWeight = max(NdotLRemap, specFloor);

    float3 lightColor  = Lights[lightIndex].color.diffuse.rgb * distanceRatio;
    float3 directLight = (directDiffColor * shadowRamp + directSpecColor * specWeight) * lightColor;

    result.diffuse = float4(directLight, 1.0f);
    
    return result;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Default Lihgting (Blinn-Phong)
LightColor CalculateLightColor(int lightIndex, float3 viewNormal, float3 viewPos)
{
    // 기존 Blinn-Phong
    LightColor color = (LightColor) 0.f;

    float3 viewLightDir = (float3) 0.f;

    float diffuseRatio = 0.f;
    float specularRatio = 0.f;
    float distanceRatio = 1.f;

    if (Lights[lightIndex].lightType == 0)
    {
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        diffuseRatio = saturate(dot(-viewLightDir, viewNormal));
    }
    else if (Lights[lightIndex].lightType == 1)
    {
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        diffuseRatio = saturate(dot(-viewLightDir, viewNormal));

        float dist = distance(viewPos, viewLightPos);
        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
            distanceRatio = saturate(1.f - pow(dist / Lights[lightIndex].range, 2));
    }
    else
    {
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos);
        diffuseRatio = saturate(dot(-viewLightDir, viewNormal));

        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
        {
            float halfAngle = Lights[lightIndex].angle / 2;

            float3 viewLightVec = viewPos - viewLightPos;
            float3 viewCenterLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);

            float centerDist = dot(viewLightVec, viewCenterLightDir);
            float lightAngle = acos(dot(normalize(viewLightVec), viewCenterLightDir));

            if (centerDist < 0.f || centerDist > Lights[lightIndex].range)
                distanceRatio = 0.f;
            else if (lightAngle > halfAngle)
                distanceRatio = 0.f;
            else
                distanceRatio = saturate(1.f - pow(centerDist / Lights[lightIndex].range, 2));
        }
    }

    float3 reflectionDir = normalize(viewLightDir + 2 * (saturate(dot(-viewLightDir, viewNormal)) * viewNormal));
    float3 eyeDir = normalize(viewPos);
    specularRatio = saturate(dot(-eyeDir, reflectionDir));
    specularRatio = pow(specularRatio, 2);

    color.diffuse = Lights[lightIndex].color.diffuse * diffuseRatio * distanceRatio;
    color.ambient = Lights[lightIndex].color.ambient * distanceRatio;
    color.specular = Lights[lightIndex].color.specular * specularRatio * distanceRatio;

    return color;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Terrain Tessellation
float CalculateTessLevel(float3 cameraWorldPos, float3 patchPos, float min, float max, float maxLv)
{
    float distance = length(patchPos - cameraWorldPos);

    if (distance < min)
        return maxLv;
    if (distance > max)
        return 1.f;

    float ratio = (distance - min) / (max - min);
    float level = (maxLv - 1.f) * (1.f - ratio);
    return level;
}



/////////////////////////////////////////////////////////////////////////////////////////
// Animation


void Skinning(inout float3 pos, inout float3 normal, inout float3 tangent,
    inout float4 weight, inout float4 indices, in uint skelBaseIdx)
{
    SkinningInfo info = (SkinningInfo) 0.f;

    for (int i = 0; i < 4; ++i)
    {
        if (weight[i] == 0.f)
            continue;

        int boneIdx = indices[i] + skelBaseIdx;
        matrix matBone = SFinalBone[boneIdx];

        info.pos += (mul(float4(pos, 1.f), matBone) * weight[i]).xyz;
        info.normal += (mul(float4(normal, 0.f), matBone) * weight[i]).xyz;
        info.tangent += (mul(float4(tangent, 0.f), matBone) * weight[i]).xyz;
    }

    pos = info.pos;
    tangent = normalize(info.tangent);
    normal = normalize(info.normal);
}
// 애니메이션 샘플링 함수 (코드 중복 제거)
void SampleAnimation(
    uint boneIndex,
    uint frameCount,
    uint currentFrame,
    uint nextFrame,
    float ratio,
    uint animOffset,
    out float4 outScale,
    out float4 outRotation,
    out float4 outTranslation)
{
    uint idx = boneIndex * frameCount + currentFrame + animOffset;
    uint nextIdx = boneIndex * frameCount + nextFrame + animOffset;
    
    outScale = lerp(AnimationClip[idx].Scale, AnimationClip[nextIdx].Scale, ratio);
    outRotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextIdx].Rotation, ratio);
    outTranslation = lerp(AnimationClip[idx].Translation, AnimationClip[nextIdx].Translation, ratio);
}

// 경계 영역 블렌딩 가중치 계산 (상하체 분리용)
float CalculateBlendWeight(uint boneIndex, uint rangeStart, uint rangeEnd, float featherRange)
{
    if (rangeEnd <= rangeStart)
        return 0.0f;
    
    float b = (float) boneIndex;
    float s = (float) rangeStart;
    float e = (float) rangeEnd;
    
    // 범위 밖이면 0
    if (b < s - featherRange || b > e + featherRange)
        return 0.0f;
    
    // 상승 구간 (rangeStart 이전부터 부드럽게 증가)
    float rise = saturate((b - (s - featherRange)) / featherRange);
    
    // 하강 구간 (rangeEnd 이후로 부드럽게 감소)
    float fall = saturate(((e + featherRange) - b) / featherRange);
    
    return rise * fall;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Shadow
uint SelectCascadeIndex(float viewDepth)
{
    viewDepth = abs(viewDepth);
    float4 splits = PassParams.CascadeSplitDistances;
    if (viewDepth <= splits.x)
        return 0;
    if (viewDepth <= splits.y)
        return 1;
    if (viewDepth <= splits.z)
        return 2;
    return RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT - 1;
}



float SampleCascadeShadow(float4 worldPos, float3 worldNormal, float3 lightDirWorld, uint cascadeIndex, out float cascadeCoverage)
{
    const float shadowMapSize = 4096.0f;

    float4 shadowClipPos = mul(worldPos, PassParams.CascadeShadowVP[cascadeIndex]);
    float invW = rcp(max(abs(shadowClipPos.w), 1e-5f));
    float3 shadowNdc = shadowClipPos.xyz * invW;

    float2 uv;
    uv.x = shadowNdc.x * 0.5f + 0.5f;
    uv.y = -shadowNdc.y * 0.5f + 0.5f;

    const float uvGuard = 0.005f;
    const float zGuard = 0.0005f;
    float2 uvMinDelta = uv - uvGuard;
    float2 uvMaxDelta = (1.0f - uvGuard) - uv;
    float uvCoverage = saturate(min(min(uvMinDelta.x, uvMinDelta.y), min(uvMaxDelta.x, uvMaxDelta.y)) / uvGuard);
    float zCoverage = saturate((shadowNdc.z - zGuard) / zGuard) * saturate(((1.0f - zGuard) - shadowNdc.z) / zGuard);
    cascadeCoverage = uvCoverage * zCoverage;

    if (cascadeCoverage <= 0.0f)
        return 1.0f;

    float3 lightDirN = normalize(lightDirWorld);
    float ndotl = saturate(dot(worldNormal, -lightDirN));

   
    float xScale = length(float3(
        PassParams.CascadeShadowVP[cascadeIndex]._11, // row1, col1
        PassParams.CascadeShadowVP[cascadeIndex]._21, // row2, col1
        PassParams.CascadeShadowVP[cascadeIndex]._31)); // row3, col1
    float texelWorldSize = 2.0f / (shadowMapSize * max(xScale, 1e-5f));

  
    float4 biasedWorldPos = float4(worldPos.xyz, 1.0f);

    float4 biasedClip = mul(biasedWorldPos, PassParams.CascadeShadowVP[cascadeIndex]);
    float biasedInvW = rcp(max(abs(biasedClip.w), 1e-5f));
    float3 biasedNdc = biasedClip.xyz * biasedInvW;

    float2 sampleUvBase;
    sampleUvBase.x = biasedNdc.x * 0.5f + 0.5f;
    sampleUvBase.y = -biasedNdc.y * 0.5f + 0.5f;
    float lightDepth = saturate(biasedNdc.z);

    float zScale = length(float3(
        PassParams.CascadeShadowVP[cascadeIndex]._13,
        PassParams.CascadeShadowVP[cascadeIndex]._23,
        PassParams.CascadeShadowVP[cascadeIndex]._33));


    float sinTheta = sqrt(max(0.0f, 1.0f - ndotl * ndotl));
    float slopeFactor = sinTheta / max(ndotl, 0.02f);
  
    float worldDepthBias = texelWorldSize * (0.5f + 1.5f * slopeFactor);
    
    
    float bias = worldDepthBias * zScale;

    float2 texelSize = 1.0f / shadowMapSize;
    float shadow = 0.0f;
    float weightSum = 0.0f;
    float compareDepth = lightDepth - bias;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y);
            float weight = 1.0f / (1.0f + dot(offset, offset));
            float2 sampleUv = sampleUvBase + offset * texelSize;
            float shadowVal = ShadowMaps.SampleCmpLevelZero(g_sam_shadow, float3(sampleUv, cascadeIndex), compareDepth);
            shadow += shadowVal * weight;
            weightSum += weight;
        }
    }

    shadow /= max(weightSum, 1e-4f);
    return 1.0f - shadow * 0.75f;
}

float CalculateCSMShadow(float3 viewPos, float3 viewNormal, float3 lightDirWorld)
{
    float viewDepth = abs(viewPos.z);

    if (viewDepth > PassParams.CascadeSplitDistances.w)
        return 1.0f;

    uint cascadeIndex = SelectCascadeIndex(viewDepth);

    float4 worldPos = mul(float4(viewPos, 1.f), PassParams.MatViewInv);
    float3 worldNormal = normalize(mul(float4(viewNormal, 0.f), PassParams.MatViewInv).xyz);

    float3 lightDirN = normalize(lightDirWorld);
    float ndotl = dot(worldNormal, -lightDirN);
    float shadowFade = smoothstep(0.0f, 0.1f, ndotl);

    if (shadowFade < 0.001f)
        return 0.0f;

    float currentCoverage = 0.0f;
    float visibility = SampleCascadeShadow(worldPos, worldNormal, lightDirWorld, cascadeIndex, currentCoverage);

    float4 splits = PassParams.CascadeSplitDistances;

    float splitDist = splits[cascadeIndex];


    uint prevIndex = (cascadeIndex == 0u) ? 0u : (cascadeIndex - 1u);
    float prevSplit = (cascadeIndex == 0u) ? 0.0f : splits[prevIndex];

    float cascadeRange = max(splitDist - prevSplit, 1.0f);
    float blendWidth = max(2.0f, cascadeRange * 0.15f);
    float blendStart = splitDist - blendWidth;
    float depthBlend = smoothstep(blendStart, splitDist, viewDepth);

    float coverageFallback = 1.0f - currentCoverage;
    float totalBlend = saturate(max(depthBlend, coverageFallback));

    if (totalBlend > 0.0f && (cascadeIndex + 1u) < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT)
    {
        float nextCoverage = 0.0f;
        float nextVisibility = SampleCascadeShadow(worldPos, worldNormal, lightDirWorld, cascadeIndex + 1, nextCoverage);
        float validBlend = totalBlend * nextCoverage;
        visibility = lerp(visibility, nextVisibility, validBlend);
    }

    return visibility * shadowFade;
}


/////////////////////////////////////////////////////////////////////////////////////////
// IBL (Image-Based Lighting)

// roughness를 고려한 Fresnel — 환경광에서 F 계산 시 사용
// FresnelSchlick과 달리 최대값을 (1 - roughness)로 제한해 거친 표면에서 과도한 반사를 방지
float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 maxVal = max(float3(1.0f - roughness, 1.0f - roughness, 1.0f - roughness), F0);
    return F0 + (maxVal - F0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// IBL 계산 결과 — diffuse와 specular를 분리해서 반환
// diffuse  DiffuseLightTarget (final pass에서 albedo와 곱해짐)
// specular  SpecularLightTarget (final pass에서 albedo 없이 그대로 더해짐)
struct IBLResult
{
    float3 diffuse;
    float3 specular;
};

// IBL ambient 기여 계산
// N_view, V_view: view-space 법선/시선 벡터
// baseColor, metallic, roughness: G-Buffer에서 읽은 PBR 파라미터
IBLResult CalculateIBLAmbient(float3 N_view, float3 V_view,
                               float3 baseColor, float metallic, float roughness)
{
    IBLResult result = (IBLResult) 0;

    // IBL 텍스처가 준비되지 않으면 기여 없음
    if (PassParams.IrradianceIndex < 0 ||
        PassParams.PreFilteredEnvIndex < 0 ||
        PassParams.BrdfLutIndex < 0)
        return result;

    // view-space → world-space 변환
    // 기존 코드에서 mul(dir, MatView) = world→view 이므로, 역방향은 MatViewInv 사용
    float3 N = normalize(mul(float4(N_view, 0.f), PassParams.MatViewInv).xyz);
    float3 V = normalize(mul(float4(V_view, 0.f), PassParams.MatViewInv).xyz);
    float3 R = reflect(-V, N); // specular IBL 샘플링에 사용할 반사 방향

    float NdotV = saturate(dot(N, V));
    roughness = max(roughness, 0.04f);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    // --- Diffuse IBL ---
    // Irradiance Map: 반구 전체 radiance의 cosine-weighted 적분 결과
    // DiffuseLightTarget에 들어가므로 baseColor 포함 (final: albedo * diffuseIBL)
    float3 irradiance = CubeBoxMaps[PassParams.IrradianceIndex].SampleLevel(g_sam_0, N, 0.0f).rgb;
    result.diffuse = kD * irradiance;

    // --- Specular IBL ---
    // Pre-filtered Map: roughness에 따른 mip 레벨로 샘플링 (split-sum 근사의 Li 항)
    // SpecularLightTarget에 들어가므로 albedo 곱 없이 반환 (final: + specularIBL)
    float mipLevel = roughness * (float) (PassParams.PreFilteredMipLevels - 1);
    float3 prefilteredColor = CubeBoxMaps[PassParams.PreFilteredEnvIndex].SampleLevel(g_sam_0, R, mipLevel).rgb;

    // BRDF LUT: (NdotV, roughness) → (scale, bias) — split-sum 근사의 BRDF 적분 항
    // g_sam_Terrain(s1)은 CLAMP 모드로 LUT 경계 오류를 방지
    float2 brdf = TextureMaps[PassParams.BrdfLutIndex].Sample(
        g_sam_Terrain, float2(NdotV, 1.0f - roughness)).rg; // [수정 2]
    result.specular = prefilteredColor * (F * brdf.x + brdf.y); // [수정 1]
    return result;
}

#endif
