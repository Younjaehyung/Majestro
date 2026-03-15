#include "params.hlsl"
#include "utils.hlsl"

// POST_MOTIONBLUR_PASS = 8 (PASS_CUSTOM_INDEX 열거형 순서와 동기화)
#define POST_MOTIONBLUR_PASS_IDX 8
#define MOTION_BLUR_SAMPLES      16
#define MAX_BLUR_RADIUS          0.02f

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;

    PASS_CUSTOM_DATA data = PassCustomTable[POST_MOTIONBLUR_PASS_IDX];

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
    float2 screenCenter = float2(0.3f, 0.5f);
    float distFromCenter = length(uv - screenCenter); // 0 ~ ~0.7
    float centerMask = smoothstep(0.0f, 0.4f, distFromCenter);
    velocity *= centerMask;
    
    // velocity 방향으로 현재 픽셀 중심 기준 양방향 샘플 누적
    float4 color = float4(0.f, 0.f, 0.f, 0.f);
    for (int i = 0; i < MOTION_BLUR_SAMPLES; ++i)
    {
        float t = (float)i / (float)(MOTION_BLUR_SAMPLES - 1); // 0 ~ 1
        float2 sampleUV = uv + velocity * (t - 0.5f);          // 중앙 기준 양방향
        sampleUV = saturate(sampleUV);
        color += Gbuffer[data.PreviousStep].Sample(g_sam_0, sampleUV);
    }

    return color / (float)MOTION_BLUR_SAMPLES;
}
