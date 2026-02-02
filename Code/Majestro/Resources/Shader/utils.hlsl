#ifndef _UTILS_HLSL_
#define _UTILS_HLSL_


#include "params.hlsl"

// -----------------------------
// [추가] PBR 보조 함수들
// -----------------------------
static const float PI = 3.14159265f;

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

// -----------------------------
// [추가] PBR 라이트 계산
// -----------------------------
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


// -----------------------------
// (기존 함수들은 그대로 둬도 됨)
// -----------------------------
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

float Rand(float2 co)
{
    return 0.5 + (frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453)) * 0.5;
    
    //frac : 소수점 추출
}

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

bool IsExactIdentity(float4x4 M)
{
    return all(M[0] == float4(1, 0, 0, 0)) &&
           all(M[1] == float4(0, 1, 0, 0)) &&
           all(M[2] == float4(0, 0, 1, 0)) &&
           all(M[3] == float4(0, 0, 0, 1));
}


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
#endif