#ifndef _UTILS_HLSL_
#define _UTILS_HLSL_


#include "params.hlsl"
#include "math.hlsl"


/////////////////////////////////////////////////////////////////////////////////////////
// ACES 톤매핑
float3 TonemapACES(float3 x)
{
    // Narkowicz ACES approximation
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

/////////////////////////////////////////////////////////////////////////////////////////
// Toon Shading


// [추가] Toon/Custom BRDF용 라이트 평가 결과
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
// [추가] 전신용 Toon 파라미터(임시 상수)
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



/////////////////////////////////////////////////////////////////////////////////////////
// PBR


float3 FresnelSchlick(float cosTheta, float3 F0)
{
    // [추가] Schlick Fresnel
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float DistributionGGX(float NdotH, float roughness)
{
    // [추가] GGX / Trowbridge-Reitz NDF
    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom + 1e-6f);
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    // [추가] Schlick-GGX geometry term (k formulation)
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f; // UE4에서 흔히 쓰는 근사

    return NdotX / (NdotX * (1.0f - k) + k + 1e-6f);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    // [추가] Smith geometry (separable)
    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

LightColor CalculateLightColorPBR(int lightIndex, float3 viewNormal, float3 viewPos,
                                  float3 baseColor, float metallic, float roughness)
{
    LightColor color = (LightColor) 0.f;

    float3 viewLightDir = (float3) 0.f; // "light -> surface" 방향으로 유지(너 기존과 동일)
    float distanceRatio = 1.f;
    float NdotL = 0.f;

    // -----------------------------
    // 1) 기존과 동일하게 라이트 방향/감쇠 계산
    // -----------------------------
    if (Lights[lightIndex].lightType == 0)
    {
        // Directional
        viewLightDir = normalize(mul(float4(Lights[lightIndex].direction.xyz, 0.f), PassParams.MatView).xyz);
        NdotL = saturate(dot(-viewLightDir, viewNormal)); // L = -viewLightDir
    }
    else if (Lights[lightIndex].lightType == 1)
    {
        // Point
        float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz;
        viewLightDir = normalize(viewPos - viewLightPos); // light -> surface
        NdotL = saturate(dot(-viewLightDir, viewNormal)); // surface -> light

        float dist = distance(viewPos, viewLightPos);
        if (Lights[lightIndex].range == 0.f)
            distanceRatio = 0.f;
        else
            distanceRatio = saturate(1.f - pow(dist / Lights[lightIndex].range, 2));
    }
    else
    {
        // Spot (너 기존 로직 유지)
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

    // 라이트가 닿지 않으면 early-out
    if (NdotL <= 0.0f || distanceRatio <= 0.0f)
    {
        color.ambient = Lights[lightIndex].color.ambient * distanceRatio;
        return color;
    }

    // -----------------------------
    // 2) PBR BRDF (Cook-Torrance GGX)
    // -----------------------------
    float3 N = normalize(viewNormal);

    // [수정] view-space에서 카메라는 원점, viewPos는 "카메라->픽셀" 방향 위치로 쓰는 게 일반적
    // V는 "픽셀->카메라" 방향이므로 -viewPos
    float3 V = normalize(-viewPos);

    // 너 기존 정의에 맞춰 L = surface->light = -viewLightDir
    float3 L = normalize(-viewLightDir);

    float3 H = normalize(V + L);

    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // [중요] roughness 0 근처에서 스파이크/NaN 방지
    roughness = max(roughness, 0.04f);

    // F0: 비금속은 0.04, 금속은 baseColor
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), baseColor, metallic);

    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    float3 F = FresnelSchlick(VdotH, F0);

    float3 numerator = D * G * F;
    float denom = max(4.0f * NdotV * NdotL, 1e-6f);
    float3 specular = numerator / denom;

    // Diffuse는 금속에서 사라짐
    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 diffuse = (kD * baseColor) / PI;

    // -----------------------------
    // 3) 라이트 색/세기 적용 (너 구조에 맞게 diffuse/specular로 분리)
    // -----------------------------
    float3 lightDiffuseRGB = Lights[lightIndex].color.diffuse.rgb;
    float3 lightSpecularRGB = Lights[lightIndex].color.specular.rgb;

    float3 radianceD = lightDiffuseRGB * distanceRatio;
    float3 radianceS = lightSpecularRGB * distanceRatio;

    float3 outDiffuse = diffuse * radianceD * NdotL * 10.f; // 라이트가 약한 파이프라인이면 이쪽이 편함

   // float3 outDiffuse  = diffuse  * radianceD * NdotL;
    float3 outSpecular = specular * radianceS * NdotL;

    color.diffuse = float4(outDiffuse, 1.0f);
    color.specular = float4(outSpecular, 1.0f);

    // ambient는 기존 파이프라인 유지 (IBL을 아직 안 한다는 전제)
    color.ambient = Lights[lightIndex].color.ambient * distanceRatio;

    return color;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Default Lihgting (Blinn-Phong)
LightColor CalculateLightColor(int lightIndex, float3 viewNormal, float3 viewPos)
{
    // 기존 Blinn-Phong ... (너 코드 그대로)
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
    return 3;
}



float SampleCascadeShadow(float4 worldPos, float3 worldNormal, float3 lightDirWorld, uint cascadeIndex, out float cascadeCoverage)
{
    float4 shadowClipPos = mul(worldPos, PassParams.CascadeShadowVP[cascadeIndex]);
    float invW = rcp(max(abs(shadowClipPos.w), 1e-5f));
    float3 shadowNdc = shadowClipPos.xyz * invW;
    
    // NDC -> UV 변환
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

    float lightDepth = saturate(shadowNdc.z);

    // bias 계산
   
    float ndotl = saturate(dot(worldNormal, -normalize(lightDirWorld)));
    float cascadeBiasScale = 1.0f + cascadeIndex * 0.25f;
    float bias = max(0.00001f, 0.00008f * (1.0f - ndotl)) * cascadeBiasScale;


    const float shadowMapSize = 4096.0f;
    float2 texelSize = 1.0f / shadowMapSize;

    float shadow = 0.0f;
    float weightSum = 0.0f;

    if (cascadeIndex == 0)
    {
        // 근거리 cascade: 5x5 
        [unroll]
        for (int y = -2; y <= 2; ++y)
        {
            [unroll]
            for (int x = -2; x <= 2; ++x)
            {
                float2 offset = float2(x, y);
                float weight = 1.0f / (1.0f + dot(offset, offset));
                float2 sampleUv = saturate(uv + offset * texelSize);
                float shadowDepth = ShadowMaps.SampleLevel(g_sam_Terrain, float3(sampleUv, cascadeIndex), 0).r;
                shadow += ((shadowDepth > 0.0f && lightDepth - bias > shadowDepth) ? 1.0f : 0.0f) * weight;
                weightSum += weight;
            }
        }
    }
    else
    {
        // 원거리 cascade: 3x3 
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            [unroll]
            for (int x = -1; x <= 1; ++x)
            {
                float2 offset = float2(x, y);
                float weight = 1.0f / (1.0f + dot(offset, offset));
                float2 sampleUv = saturate(uv + offset * texelSize);
                float shadowDepth = ShadowMaps.SampleLevel(g_sam_Terrain, float3(sampleUv, cascadeIndex), 0).r;
                shadow += ((shadowDepth > 0.0f && lightDepth - bias > shadowDepth) ? 1.0f : 0.0f) * weight;
                weightSum += weight;
            }
        }
    }

    shadow /= max(weightSum, 1e-4f);
    return 1.0f - shadow * 0.75f;
}

float CalculateCSMShadow(float3 viewPos, float3 viewNormal, float3 lightDirWorld)
{
    uint cascadeIndex = SelectCascadeIndex(viewPos.z);
    float viewDepth = abs(viewPos.z);

    float4 worldPos = mul(float4(viewPos, 1.f), PassParams.MatViewInv);
    float3 worldNormal = normalize(mul(float4(viewNormal, 0.f), PassParams.MatViewInv).xyz);

    float currentCoverage = 0.0f;
    float visibility = SampleCascadeShadow(worldPos, worldNormal, lightDirWorld, cascadeIndex, currentCoverage);

    float4 splits = PassParams.CascadeSplitDistances;

    // 캐스케이드 경계 블렌딩 틈새 방지
    if (cascadeIndex < 3)
    {
        float splitDist = splits[cascadeIndex];
        float prevSplit = (cascadeIndex == 0) ? 0.0f : splits[cascadeIndex - 1];
        float cascadeRange = max(splitDist - prevSplit, 1.0f);
        float blendWidth = max(2.0f, cascadeRange * 0.15f);

        float blendStart = splitDist - blendWidth;
        float depthBlend = smoothstep(blendStart, splitDist, viewDepth);

        // currentCoverage = 0 다음 cascade로
        float coverageFallback = 1.0f - currentCoverage;
        float totalBlend = saturate(max(depthBlend, coverageFallback));

        if (totalBlend > 0.0f)
        {
            float nextCoverage = 0.0f;
            float nextVisibility = SampleCascadeShadow(worldPos, worldNormal, lightDirWorld, cascadeIndex + 1, nextCoverage);
            // 다음 cascade에 coverage가 있을 때만 블렌딩
            float validBlend = totalBlend * nextCoverage;
            visibility = lerp(visibility, nextVisibility, validBlend);
        }
    }
    else // cascadeIndex == 3
    {
        float splitFar = splits.w;
        float fadeStart = splitFar * 0.85f; // 마지막 15% 구간에서 서서히 사라짐
        float depthFade = 1.0f - smoothstep(fadeStart, splitFar, viewDepth);
        // coverage가 낮은 영역
        float blendFactor = depthFade * currentCoverage;
        visibility = lerp(1.0f, visibility, blendFactor);
    }

    return visibility;
}

#endif