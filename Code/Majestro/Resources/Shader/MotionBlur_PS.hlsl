#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

#define POST_MOTIONBLUR_PASS_IDX 8
#define MOTION_BLUR_SAMPLES      16
#define MAX_BLUR_RADIUS          0.02f
#define SPEED_LINE_SLOTS         28
#define SPEED_LINE_LAYERS        3
#define SPEED_LINE_SHARPNESS     1.0f

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float4 sourceColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, uv);
    float4 result = sourceColor;

    int mvIdx = data.ExtTex[0];
    float2 velocity = Gbuffer[mvIdx].Sample(g_sam_0, uv).rg;


    // 모션 블러
    if (length(velocity) >= 0.0001f)
    {
        float speed = length(velocity);
        if (speed > MAX_BLUR_RADIUS)
            velocity = velocity / speed * MAX_BLUR_RADIUS;

        // 중앙 억제 마스크 추가 — 중앙=0, 주변=1
        float2 screenCenter = float2(0.5f, 0.5f);
        float distFromCenter = length(uv - screenCenter);
        float centerMask = smoothstep(0.0f, 0.4f, distFromCenter);
        velocity *= centerMask;

        // velocity 방향으로 현재 픽셀 중심 기준 양방향 샘플 누적
        float4 color = float4(0.f, 0.f, 0.f, 0.f);
        for (int i = 0; i < MOTION_BLUR_SAMPLES; ++i)
        {
            float t = (float) i / (float) (MOTION_BLUR_SAMPLES - 1);
            float2 sampleUV = uv + velocity * (t - 0.5f);
            sampleUV = saturate(sampleUV);
            color += Gbuffer[data.PreviousStep].Sample(g_sam_0, sampleUV);
        }

        result = color / (float) MOTION_BLUR_SAMPLES;
    }

    float time = data.ExtValue[0].x;
    float dashBlend = data.ExtValue[0].y;
    float speedLineDensity = data.ExtValue[0].z;
    float speedLineIntensity = data.ExtValue[0].w;

    if (dashBlend < 0.0001f)
        return result;

    // 대시 속도선 
    float3 lineColor = data.ExtValue[1].xyz;
    float aspect = max(PassParams.ScreenSize.x / max(PassParams.ScreenSize.y, 1.0f), 0.0001f);
    float2 centered = uv - float2(0.5f, 0.5f);
    centered.x *= aspect;

    float dist = length(centered);
    float angle = atan2(centered.y, centered.x);
    float angle01 = frac(angle / (2.0f * PI) + 0.5f);
    float slotF = angle01 * (float) SPEED_LINE_SLOTS;
    float slotIndex = floor(slotF);
    float slotLocal = frac(slotF);

    
    float outerMask = smoothstep(0.28f, 0.48f, dist) * (1.0f - smoothstep(1.05f, 1.25f, dist));
    float edgeMask = smoothstep(0.58f, 0.92f, dist);
    float fade = dashBlend * dashBlend;
    float coreLine = 0.0f;
    float glowLine = 0.0f;

    [unroll]
    for (int layer = 0; layer < SPEED_LINE_LAYERS; ++layer)
    {
        float seed = slotIndex + (float) layer * 97.13f;
        float rnd0 = Hash(seed + floor(time * 10.0f) * 13.0f);
        float rnd1 = Hash(seed + 23.71f);
        float rnd2 = Hash(seed + 41.37f);
        float rnd3 = Hash(seed + 61.53f);
        float exists = step(1.0f - speedLineDensity, rnd0);

        float angularWidth = lerp(0.018f, 0.055f, rnd1) * SPEED_LINE_SHARPNESS;
        float angularDelta = abs(slotLocal - 0.5f);
        float angularCore = 1.0f - smoothstep(angularWidth * 0.18f, angularWidth, angularDelta);
        float angularGlow = 1.0f - smoothstep(angularWidth, angularWidth * 3.5f, angularDelta);

        float travel = frac(time * lerp(0.85f, 1.65f, rnd2) + rnd3);
        float head = lerp(0.54f, 1.18f, travel);
        float lengthValue = lerp(0.24f, 0.44f, rnd1);
        float tail = head - lengthValue;

        float radialFront = 1.0f - smoothstep(head - 0.035f, head, dist);
        float radialBack = smoothstep(tail, tail + lengthValue * 0.42f, dist);
        float radialMask = saturate(radialFront * radialBack);

        float highlight = pow(saturate((dist - tail) / max(lengthValue, 0.001f)), 2.2f);
        coreLine += angularCore * radialMask * highlight * exists;
        glowLine += angularGlow * radialMask * exists;
    }

    coreLine *= outerMask * edgeMask * speedLineIntensity * fade;
    glowLine *= outerMask * speedLineIntensity * fade * 0.55f;

    result.rgb += lineColor * glowLine * 0.45f;
    result.rgb += lineColor * coreLine * 1.35f;

    return result;
}
