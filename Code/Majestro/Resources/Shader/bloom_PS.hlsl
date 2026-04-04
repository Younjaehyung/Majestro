#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

static const float2 BLOOM_OFFSETS[13] =
{
    // 수정: 중심 샘플 (0, 0) 추가
    float2(0.f, 0.f),

    float2(-1.f, -1.f), float2(0.f, -1.f), float2(1.f, -1.f),
    float2(-1.f, 0.f), float2(1.f, 0.f),
    float2(-1.f, 1.f), float2(0.f, 1.f), float2(1.f, 1.f),
    float2(-2.f, 0.f), float2(2.f, 0.f),
    float2(0.f, -2.f), float2(0.f, 2.f),
};

static const float BLOOM_WEIGHTS[13] =
{
    // 수정: 중심 샘플 가중치 추가
    0.20f,

    0.05f, 0.08f, 0.05f,
    0.08f, 0.08f,
    0.05f, 0.08f, 0.05f,
    0.04f, 0.04f,
    0.04f, 0.04f,
};

// 수정: hard threshold 대신 soft knee를 사용하는 bloom factor 함수 추가
float ComputeBloomFactor(float brightness, float threshold, float knee)
{
    float soft = brightness - threshold + knee;
    soft = clamp(soft, 0.0f, 2.0f * knee);
    soft = (soft * soft) / max(4.0f * knee, 1e-5f);

    float contribution = max(soft, brightness - threshold);
    return contribution / max(brightness, 1e-5f);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float2 texelSize = 1.0f / float2(PassParams.ScreenSize.x, PassParams.ScreenSize.y);

    float threshold = data.ExtValue[0].x;
    float intensity = data.ExtValue[0].y;
    float radius = data.ExtValue[0].z;

    // 수정: soft knee 값 추가.
    // 지금 파라미터 구조상 ExtValue[0].w가 남아 있으면 거기에 넣는 걸 권장.
    float knee = 1.0f;

    float3 bloom = 0.f;
    float totalWeight = 0.f;

    [unroll]
    for (int i = 0; i < 13; ++i)
    {
        float2 sampleUV = input.uv + BLOOM_OFFSETS[i] * texelSize * radius;

        // 수정: UV 범위 보정. sampler가 clamp가 아니어도 안전하게 만듦.
        sampleUV = saturate(sampleUV);

        float3 s = Gbuffer[data.PreviousStep].Sample(g_sam_0, sampleUV).rgb;

        // 수정: luminance 대신 max channel 기반 brightness 사용
        // stylized emissive / 네온 / 강한 발광 표현에서 더 직관적임
        float brightness = max(s.r, max(s.g, s.b));

        // 수정: soft knee 기반 추출
        float bloomFactor = ComputeBloomFactor(brightness, threshold, knee);

        bloom += s * bloomFactor * BLOOM_WEIGHTS[i];
        totalWeight += BLOOM_WEIGHTS[i];
    }

    bloom /= max(totalWeight, 1e-5f);

    // 중요:
    // 수정: 여기서는 bloom만 출력하는 것이 더 바람직하다.
    // 현재 구조를 유지하려면 아래처럼 원본 합성도 가능하지만,
    // 실무적으로는 composite pass로 분리하는 것을 권장.
    float3 original = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv).rgb;

    // 임시 유지 버전
    return float4(original + bloom * intensity, 1.f);

    // 권장 버전:
    // return float4(bloom * intensity, 1.f);
}