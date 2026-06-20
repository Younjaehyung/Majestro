#include "params.hlsl"

// ──────────────────────────────────────────────────────────────
// FXAA 3.11 (단순화) — LDR 포스트프로세스 앤티얼라이싱
//
// PassCustomTable[POST_FXAA_PASS = 20]:
//   PreviousStep  : 입력 LDR RT의 Gbuffer[] 인덱스
//                   (POST_LDR_A = GBUFFER_POSTC_INDEX = 10,
//                    POST_LDR_B = GBUFFER_POSTD_INDEX = 11)
//   ExtValue[0].x : edgeThreshold     (기본 0.125  — 엣지 감지 최소 루마 대비)
//   ExtValue[0].y : edgeThresholdMin  (기본 0.0625 — 어두운 영역 컷오프)
//   ExtValue[0].z : subpixQuality     (기본 0.75   — 서브픽셀 블렌드 강도)
//
// 알고리즘:
//   1. 5-tap 루마 샘플로 엣지 대비 감지
//   2. 9-tap으로 수평/수직 엣지 방향 판별
//   3. 엣지 따라 최대 FXAA_SEARCH_STEPS(=12) 양방향 탐색 → 엔드포인트
//   4. 엣지 길이 기반 픽셀 오프셋 블렌드
//   5. 3×3 서브픽셀 앤티얼라이싱 블렌드 합산
//
// 샘플러: g_sam_0 (Anisotropic WRAP, s0)
//   FXAA는 Bilinear 이상이면 동작. UV는 saturate()로 직접 클램프.
// ──────────────────────────────────────────────────────────────

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// 최대 엔드포인트 탐색 스텝 (품질↑ = 스텝 수↑, 성능↓)
// 식생 많은 씬은 엣지가 짧아 긴 탐색 불필요 → 12→6으로 최악 탭 반감 (긴 직선 엣지만 살짝 영향)
static const int   FXAA_SEARCH_STEPS = 6;

// Rec.2002 루마 계수
float Luma(float3 rgb)
{
    return dot(rgb, float3(0.2627f, 0.6780f, 0.0593f));
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint  srcIdx           = (uint) data.PreviousStep;
    float edgeThreshold    = data.ExtValue[0].x;
    float edgeThresholdMin = data.ExtValue[0].y;
    float subpixQuality    = data.ExtValue[0].z;

    float2 texelSize = float2(1.0f / PassParams.ScreenSize.x,
                              1.0f / PassParams.ScreenSize.y);
    float2 uv = input.uv;

    // ── 1. 현재 픽셀 색상 및 상하좌우 루마 샘플 ──────────────────
    float3 colorM = Gbuffer[srcIdx].Sample(g_sam_0, uv).rgb;
    float  lumaM  = Luma(colorM);

    float lumaN  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2( 0.0f,         -texelSize.y))).rgb);
    float lumaS  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2( 0.0f,         +texelSize.y))).rgb);
    float lumaE  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x,   0.0f       ))).rgb);
    float lumaW  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x,   0.0f       ))).rgb);

    float lumaMax   = max(max(lumaN, lumaS), max(lumaE, max(lumaW, lumaM)));
    float lumaMin   = min(min(lumaN, lumaS), min(lumaE, min(lumaW, lumaM)));
    float lumaRange = lumaMax - lumaMin;

    // ── 2. 엣지 감지: 대비가 임계값 미만이면 FXAA 적용 안 함 ─────
    if (lumaRange < max(edgeThresholdMin, lumaMax * edgeThreshold))
        return float4(colorM, 1.0f);

    // ── 3. 대각선 이웃 루마 (수평/수직 판별 및 서브픽셀용) ────────
    float lumaNW = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x, -texelSize.y))).rgb);
    float lumaNE = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x, -texelSize.y))).rgb);
    float lumaSW = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x, +texelSize.y))).rgb);
    float lumaSE = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x, +texelSize.y))).rgb);

    // ── 4. 수평/수직 엣지 판별 (Sobel-like 그라디언트 추정) ───────
    //   수평 강도: 세로 방향 루마 차이 합산
    //   수직 강도: 가로 방향 루마 차이 합산
    float edgeH = abs(lumaNW + lumaN + lumaNE - lumaSW - lumaS - lumaSE) * 2.0f
                + abs(lumaNW - lumaSW) + abs(lumaNE - lumaSE);
    float edgeV = abs(lumaNW + lumaW + lumaSW - lumaNE - lumaE - lumaSE) * 2.0f
                + abs(lumaNW - lumaNE) + abs(lumaSW - lumaSE);

    bool isHorizontal = (edgeH >= edgeV);

    // ── 5. 엣지 수직 방향 그라디언트로 이동 방향 결정 ────────────
    float luma1 = isHorizontal ? lumaN : lumaW;
    float luma2 = isHorizontal ? lumaS : lumaE;

    float gradient1 = abs(luma1 - lumaM);
    float gradient2 = abs(luma2 - lumaM);

    bool  is1Steeper     = (gradient1 >= gradient2);
    float gradientScaled = 0.25f * max(gradient1, gradient2);

    float stepSize     = isHorizontal ? texelSize.y : texelSize.x;
    float lumaLocalAvg;

    if (is1Steeper)
    {
        stepSize    = -stepSize;
        lumaLocalAvg = 0.5f * (luma1 + lumaM);
    }
    else
    {
        lumaLocalAvg = 0.5f * (luma2 + lumaM);
    }

    // ── 6. 엣지 중심점으로 0.5 텍셀 이동 ─────────────────────────
    float2 currentUV = uv;
    if (isHorizontal) currentUV.y += stepSize * 0.5f;
    else              currentUV.x += stepSize * 0.5f;

    // ── 7. 엣지 방향 탐색 오프셋 설정 ────────────────────────────
    float2 offset = isHorizontal ? float2(texelSize.x, 0.0f)
                                 : float2(0.0f, texelSize.y);

    float2 uv1 = currentUV - offset;
    float2 uv2 = currentUV + offset;

    float lumaEnd1 = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv1)).rgb) - lumaLocalAvg;
    float lumaEnd2 = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv2)).rgb) - lumaLocalAvg;

    bool reached1 = (abs(lumaEnd1) >= gradientScaled);
    bool reached2 = (abs(lumaEnd2) >= gradientScaled);

    // ── 8. 엔드포인트까지 양방향 탐색 ────────────────────────────
    [loop]
    for (int i = 0; i < FXAA_SEARCH_STEPS && !(reached1 && reached2); ++i)
    {
        if (!reached1)
        {
            uv1      -= offset;
            lumaEnd1  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv1)).rgb) - lumaLocalAvg;
            reached1  = (abs(lumaEnd1) >= gradientScaled);
        }
        if (!reached2)
        {
            uv2      += offset;
            lumaEnd2  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv2)).rgb) - lumaLocalAvg;
            reached2  = (abs(lumaEnd2) >= gradientScaled);
        }
    }

    // ── 9. 엣지 길이 기반 픽셀 오프셋 계산 ──────────────────────
    float dist1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float dist2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);

    float edgeLen     = dist1 + dist2;
    float pixelOffset = -min(dist1, dist2) / edgeLen + 0.5f;

    // 루마가 로컬 평균보다 낮고 엔드포인트 방향이 반전이면 오프셋 유효
    bool lumaMLessAvg  = (lumaM < lumaLocalAvg);
    bool correctVar1   = ((lumaEnd1 < 0.0f) != lumaMLessAvg);
    bool correctVar2   = ((lumaEnd2 < 0.0f) != lumaMLessAvg);
    bool correctVar    = (dist1 < dist2) ? correctVar1 : correctVar2;

    float pixelOffsetFinal = correctVar ? pixelOffset : 0.0f;

    // ── 10. 서브픽셀 앤티얼라이싱 블렌드 팩터 ────────────────────
    //   3×3 인근 평균 루마와 현재 픽셀 루마의 차이 기반
    float lumaAvg3x3 = (1.0f / 12.0f) * (
        2.0f * (lumaN + lumaS + lumaE + lumaW) +
        lumaNW + lumaNE + lumaSW + lumaSE);

    float subpixOffset1 = saturate(abs(lumaAvg3x3 - lumaM) / lumaRange);
    float subpixOffset2 = subpixOffset1 * subpixOffset1 * subpixQuality;

    // 엣지 오프셋과 서브픽셀 오프셋 중 큰 값 선택
    float finalOffset = max(pixelOffsetFinal, subpixOffset2);

    // ── 11. 최종 색상 샘플링 ──────────────────────────────────────
    float2 finalUV = uv;
    if (isHorizontal) finalUV.y += finalOffset * stepSize;
    else              finalUV.x += finalOffset * stepSize;

    finalUV = saturate(finalUV);

    return float4(Gbuffer[srcIdx].Sample(g_sam_0, finalUV).rgb, 1.0f);
}
