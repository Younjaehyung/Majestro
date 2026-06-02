#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint srcIdx = (uint) data.PreviousStep;
    float time = data.ExtValue[0].x;
    float lowStrength = saturate(data.ExtValue[0].y);
    float healStrength = saturate(data.ExtValue[0].z);
    float noiseTiling = max(data.ExtValue[0].w, 0.001f);
    int noiseTexIdx = data.ExtTex[0];

    // deal
    float3 lowColor = data.ExtValue[1].rgb;
    float lowPulseSpeed = data.ExtValue[1].w;
    
    // heal
    float3 healColor = data.ExtValue[2].rgb;
    float healPulseSpeed = data.ExtValue[2].w;

    float maskStart = data.ExtValue[3].x;
    float maskEnd = data.ExtValue[3].y;
    float waveAmount = data.ExtValue[3].z;
    float waveFrequency = data.ExtValue[3].w;

    float effectStrength = saturate(max(lowStrength, healStrength));
    if (effectStrength <= 0.0001f)
        return Gbuffer[srcIdx].Sample(g_sam_0, input.uv);

    float2 centered = input.uv - float2(0.5f, 0.5f);
    float aspect = max(PassParams.ScreenSize.x / max(PassParams.ScreenSize.y, 1.0f), 0.0001f);
    centered.x *= aspect;

    float dist = length(centered);
    float angle = atan2(centered.y, centered.x);
    
    // 화면 중앙은 유지하고 외곽부만 부드럽게 켜는 비네팅 마스크 
    float edgeMask = smoothstep(maskStart, maskEnd, dist);

    // 외곽 마스크에 시간 파형과 거친 셀 노이즈를 섞어 일렁임
    float waveA = sin(time * 2.7f + dist * waveFrequency + angle * 3.0f);
    float waveB = sin(time * 1.6f - dist * (waveFrequency * 0.7f) + angle * 7.0f);
    float wave = 0.5f + 0.5f * (waveA * 0.65f + waveB * 0.35f);
    float textureDirty = 0.f;

    // ExtTex[0]에 노이즈 텍스처 = 얼룩 노이즈
    if (noiseTexIdx >= 0)
    {
        float2 noiseUV = input.uv * noiseTiling + float2(time * 0.035f, -time * 0.02f);
        float4 noiseSample = TextureMaps[noiseTexIdx].Sample(g_sam_0, frac(noiseUV));
        textureDirty = dot(noiseSample.rgb, float3(0.3333f, 0.3333f, 0.3334f));
    }
    else
    { // 텍스처가 없을 때만 화면 UV 기반 절차 노이즈를 사용
        float timeCell = floor(time * 7.0f);
        float2 proceduralCell = floor((input.uv + float2(time * 0.025f, -time * 0.018f)) * float2(26.0f, 15.0f));
        float proceduralDirty = Hash21(proceduralCell + timeCell);
        textureDirty = proceduralDirty;
    }

    float dirtyNoise = textureDirty;
    float dirtyPatch = lerp(0.72f, 1.34f, dirtyNoise);
    float dirtyCut = smoothstep(0.22f, 0.88f, dirtyNoise);

    float animatedMask = edgeMask * lerp(1.0f - waveAmount, 1.0f + waveAmount, wave);
    animatedMask *= lerp(0.82f, dirtyPatch, 0.55f);
    animatedMask *= lerp(0.88f, 1.18f, dirtyCut);
    animatedMask = saturate(animatedMask);

    // 일렁거림
    float4 color = Gbuffer[srcIdx].Sample(g_sam_0, input.uv);

    float lowPulse = 0.78f + 0.22f * sin(time * lowPulseSpeed);
    float healPulse = 0.70f + 0.30f * sin(time * healPulseSpeed + dist * 8.0f);

    // 피격시
    float lowMask = animatedMask * lowStrength * lowPulse;
    
    // 힐링시
    float healMask = animatedMask * healStrength * healPulse;

    // 얼룩마다 색을 조금 다르게 하기
    float3 dirtyLowColor = lerp(float3(0.95f, 0.005f, 0.0f), lowColor, dirtyNoise);
    float3 dirtyHealColor = lerp(float3(0.18f, 0.80f, 0.04f), healColor, dirtyNoise);
    float lowEdge = saturate(lowMask * 1.25f);

    // 피격 색 (색상 조금 더 강렬하게 일부러 눌렀음)
    color.rgb *= 1.0f - lowEdge * 0.28f;
    color.g *= 1.0f - lowEdge * 0.42f;
    color.b *= 1.0f - lowEdge * 0.50f;
    color.rgb += dirtyLowColor * lowEdge * 1.25f;

    // 회복 색
    color.rgb += dirtyHealColor * healMask * 0.85f;

    return color;
}
