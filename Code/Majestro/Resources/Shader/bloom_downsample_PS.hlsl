#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bloom Downsample Pass  ── 13-tap dual Kawase + Karis average
//
//  Call of Duty: Advanced Warfare (Jimenez 2014) 방식 구현.
//  단순 bilinear 대비 firefly(반짝이는 노이즈) 억제 및 aliasing 감소.
//
//  13개 샘플 위치 (각 2×2 bilinear tap 중심 기준):
//
//    a . b . c
//    . d . e .
//    f . g . h
//    . i . j .
//    k . l . m
//
//  가중치:
//    inner quad (d,e,i,j) × 4 → 각 0.5   (합 2.0)
//    outer quad (a-b-f-g 등 4개) × 4 → 각 0.125  (합 2.0)
//    총합 = 4.0 → 평균 = sum/4.0
//
//  첫 번째 다운샘플(bright→MIP0)에서만 Karis average 적용.
//  이후 단계는 생략해도 firefly가 이미 억제된 상태라 품질 차이 미미.
//
//  PassCustomTable[POST_BLOOM_DS_PASS]:
//   ExtTex[0]       : 소스 BloomMips 인덱스 (0=MIP0, 1=MIP1, 2=MIP2)
//   ExtValue[0].x   : srcTexelW = 1.0 / 소스 RT 너비
//   ExtValue[0].y   : srcTexelH = 1.0 / 소스 RT 높이
//   ExtValue[0].z   : useKaris  (1.0이면 Karis average 적용, 0이면 생략)
// ─────────────────────────────────────────────────────────────────────────────

// Karis average: HDR 에너지 과대 표현 억제 (1/(1+luma) 가중 평균)
float KarisWeight(float3 c)
{
    return 1.0f / (1.0f + dot(c, float3(0.2126f, 0.7152f, 0.0722f)));
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    // srcMip: GlobalParams.etc (0=MIP0, 1=MIP1, 2=MIP2)
    // useKaris: GlobalParams.casdcae (1 = Karis 적용)
    uint  srcMip   = GlobalParams.etc;
    float useKaris = (float)GlobalParams.casdcae;
    float2 uv      = input.uv;

    // 소스 해상도 = screenSize / 2^(srcMip+1)
    float srcScale = (float)(1u << (srcMip + 1u));
    float tw = srcScale / PassParams.ScreenSize.x;
    float th = srcScale / PassParams.ScreenSize.y;

    // 13개 샘플 (각 2×2 bilinear tap)
    float3 a = BloomMips[srcMip].Sample(g_sam_0, uv + float2(-2,-2) * float2(tw,th)).rgb;
    float3 b = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 0,-2) * float2(tw,th)).rgb;
    float3 c = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 2,-2) * float2(tw,th)).rgb;
    float3 d = BloomMips[srcMip].Sample(g_sam_0, uv + float2(-1,-1) * float2(tw,th)).rgb;
    float3 e = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 1,-1) * float2(tw,th)).rgb;
    float3 f = BloomMips[srcMip].Sample(g_sam_0, uv + float2(-2, 0) * float2(tw,th)).rgb;
    float3 g = BloomMips[srcMip].Sample(g_sam_0, uv                               ).rgb;
    float3 h = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 2, 0) * float2(tw,th)).rgb;
    float3 i = BloomMips[srcMip].Sample(g_sam_0, uv + float2(-1, 1) * float2(tw,th)).rgb;
    float3 j = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 1, 1) * float2(tw,th)).rgb;
    float3 k = BloomMips[srcMip].Sample(g_sam_0, uv + float2(-2, 2) * float2(tw,th)).rgb;
    float3 l = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 0, 2) * float2(tw,th)).rgb;
    float3 m = BloomMips[srcMip].Sample(g_sam_0, uv + float2( 2, 2) * float2(tw,th)).rgb;

    float3 result;

    if (useKaris > 0.5f)
    {
        // ── Karis average (첫 번째 다운샘플만 권장) ────────────────────
        // 4개의 inner 2×2 quad 각각 가중 평균 후 합산
        float3 q1 = (d + e + i + j) * 0.25f;
        float3 q2 = (a + b + f + g) * 0.25f;
        float3 q3 = (b + c + g + h) * 0.25f;
        float3 q4 = (f + g + k + l) * 0.25f;
        float3 q5 = (g + h + l + m) * 0.25f;

        // Karis 가중치
        float w1 = KarisWeight(q1);
        float w2 = KarisWeight(q2);
        float w3 = KarisWeight(q3);
        float w4 = KarisWeight(q4);
        float w5 = KarisWeight(q5);

        result = (q1 * w1 * 0.5f
                + q2 * w2 * 0.125f
                + q3 * w3 * 0.125f
                + q4 * w4 * 0.125f
                + q5 * w5 * 0.125f)
               / (w1 * 0.5f + (w2 + w3 + w4 + w5) * 0.125f);
    }
    else
    {
        // ── 일반 13-tap 가중 평균 ─────────────────────────────────────
        // inner quad × 0.5, outer quad × 0.125
        result = (d + e + i + j) * 0.5f
               + (a + b + f + g) * 0.125f
               + (b + c + g + h) * 0.125f
               + (f + g + k + l) * 0.125f
               + (g + h + l + m) * 0.125f;
        result *= 0.25f; // 총 가중치 4.0으로 정규화
    }

    return float4(max(result, 0.0f), 1.0f);
}
