#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bloom Bright Pass  ── HDR SceneColor에서 발광 영역 추출
//
//  [원칙]
//   - 소스는 HDR SceneColor (에미시브가 이미 합산된 상태) — LightsPass/ForwardPass
//     출력 이후의 HDR RT를 읽는다.
//   - LDR threshold가 아닌 HDR luminance 기준으로 추출하므로,
//     specular 하이라이트 · 하늘 · 에미시브 모두 bloom에 기여한다.
//   - Soft-knee: 경계 픽셀이 갑자기 잘리지 않고 부드럽게 페이드인.
//
//  PassCustomTable[POST_BLOOM_BRIGHT_PASS]:
//   PreviousStep       : HDR SceneColor GBuffer 인덱스
//   ExtValue[0].x      : threshold     — bloom 시작 휘도 (HDR 단위, 권장 1.0~1.5)
//   ExtValue[0].y      : knee          — soft-knee 폭 (권장 threshold * 0.5)
//   ExtValue[0].z      : prefilterStrength — 추출 후 곱하는 강도 (기본 1.0)
// ─────────────────────────────────────────────────────────────────────────────

static const float3 BT709_LUMA = float3(0.2126f, 0.7152f, 0.0722f);

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float threshold = data.ExtValue[0].x;
    float knee      = max(data.ExtValue[0].y, 1e-5f);
    float strength  = data.ExtValue[0].z;

    // HDR SceneColor 읽기 (에미시브 포함, tone-map 전)
    float3 hdrColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv).rgb;

    // 휘도 계산 (HDR 단위 — 1.0을 넘을 수 있음)
    float lum = dot(hdrColor, BT709_LUMA);

    // ── Soft-knee threshold ──────────────────────────────────────────────
    //  [threshold - knee, threshold + knee] 구간에서 부드럽게 ramp-up.
    //  아래 임계값 미만은 0, 위는 원본 그대로.
    //
    //   rq = clamp(lum - (threshold - knee), 0, 2*knee)
    //   weight = rq² / (4*knee)          ← 이차 커브로 부드럽게 이행
    //   최종 weight = max(rq²/(4*knee), lum - threshold) / max(lum, ε)
    // ─────────────────────────────────────────────────────────────────────
    float rq     = clamp(lum - threshold + knee, 0.0f, 2.0f * knee);
    rq           = (rq * rq) / (4.0f * knee);
    float weight = max(rq, lum - threshold) / max(lum, 1e-5f);
    weight       = saturate(weight);

    float3 extracted = hdrColor * weight * strength;

    return float4(max(extracted, 0.0f), 1.0f);
}
