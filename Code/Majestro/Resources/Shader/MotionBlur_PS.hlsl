#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

// POST_MOTIONBLUR_PASS = 8 (PASS_CUSTOM_INDEX 열거형 순서와 동기화)
#define POST_MOTIONBLUR_PASS_IDX 8
#define MOTION_BLUR_SAMPLES      16
#define MAX_BLUR_RADIUS          0.02f
#define LINE_COUNT      10          // 전체 슬롯 수 (선의 최대 밀도)
#define LINE_SHARPNESS  3.0f      // 높을수록 선이 가늘고 날카로움
struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;

    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    // MOTION_VECTOR RT에서 velocity 읽기 (ExtTex[0]에 GBUFFER_MOTIONVEC_INDEX 저장)
    int mvIdx = data.ExtTex[0];
    float2 velocity = Gbuffer[mvIdx].Sample(g_sam_0, uv).rg;

    // velocity가 충분히 작으면 blur 없이 원본 반환
    if (length(velocity) < 0.0001f)
        return Gbuffer[data.PreviousStep].Sample(g_sam_0, uv);

    // 최대 반경 클램핑
    float speed = length(velocity);
    if (speed > MAX_BLUR_RADIUS)
        velocity = velocity / speed * MAX_BLUR_RADIUS;

    // 중앙 억제 마스크 추가 — 중앙=0, 주변=1
    float2 screenCenter = float2(0.5f, 0.5f);
    float distFromCenter = length(uv - screenCenter); // 0 ~ ~0.7
    float centerMask = smoothstep(0.0f, 0.4f, distFromCenter);
    velocity *= centerMask;

// velocity 방향으로 현재 픽셀 중심 기준 양방향 샘플 누적
    float4 color = float4(0.f, 0.f, 0.f, 0.f);
    for (int i = 0; i < MOTION_BLUR_SAMPLES; ++i)
    {
        float t = (float) i / (float) (MOTION_BLUR_SAMPLES - 1); // 0 ~ 1
        float2 sampleUV = uv + velocity * (t - 0.5f); // 중앙 기준 양방향
        sampleUV = saturate(sampleUV);
        color += Gbuffer[data.PreviousStep].Sample(g_sam_0, sampleUV);
    }

    float4 result = color / (float) MOTION_BLUR_SAMPLES;

// Speed Line

    float2 Speeduv = input.uv;

    float time = data.ExtValue[0].x;
    float dashBlend = data.ExtValue[0].y;
    float speedLineDensity = data.ExtValue[0].z;
    float speedLineIntensity = data.ExtValue[0].w;
    int noiseIdx = data.ExtTex[1];


    if (dashBlend < 0.0001f)
        return result;

    float3 lineColor = float3(data.ExtValue[1].xyz);

    if (dashBlend < 0.0001f)
        return result;

    float2 centered = uv - float2(0.5f, 0.5f);
// centered.x *= aspectRatio;

    float dist = length(centered);
    float angle = atan2(centered.y, centered.x);

    float slotF = (angle / 2 * PI + 0.5f) * (float) LINE_COUNT;
    float slotIndex = floor(slotF);
    float slotLocal = frac(slotF);

    float timeSlot = floor(time * 12.0f);


    float rand0 = Hash(slotIndex + timeSlot * (float) LINE_COUNT);
    float rand1 = Hash(slotIndex + timeSlot * (float) LINE_COUNT + 1.0f);

    float lineExists = step(1.0f - speedLineDensity, rand0);

 // 굵기 변화
    float sharpness = lerp(50.0f, 20.0f, rand1);

    float peak = slotLocal * (1.0f - slotLocal) * 4.0f;
    float SR_line = pow(peak, sharpness);
    SR_line *= lineExists;

 // 거리 마스킹
    SR_line *= smoothstep(0.35f, 0.45f, dist);
    SR_line *= smoothstep(0.75f, 0.45f, dist);

 // 글로우 레이어

    float glowSharpness = max(1.0f, sharpness * 0.08f);
    float glow = pow(peak, glowSharpness);
    glow *= lineExists;
    glow *= smoothstep(0.30f, 0.42f, dist);
    glow *= smoothstep(0.58f, 0.44f, dist);
    glow *= 0.4f; // 글로우는 선보다 어둡게

 // 잔상 느낌
    float fade = dashBlend * dashBlend;

    SR_line *= speedLineIntensity * fade;
    glow *= speedLineIntensity * fade;


    result.rgb += lineColor * glow * 0.4f; 
    result.rgb = lerp(result.rgb, lineColor * 1.5f, saturate(SR_line));


    return result;
}
