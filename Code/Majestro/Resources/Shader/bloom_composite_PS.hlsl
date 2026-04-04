#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bloom Composite Pass  ── FinalHDR = HDR SceneColor + Bloom
//
//  [합성 순서]
//   1. HDR SceneColor 읽기  (Lighting + Emissive가 합산된 상태)
//   2. BloomMips[0]  읽기  (피라미드 업샘플 누적 완료된 1/2 해상도 bloom)
//   3. result = hdr + bloom * bloomIntensity
//   4. 이 결과가 ToneMapPass의 입력이 됨 → Exposure → ACES → LDR
//
//  Bloom은 반드시 Tone-map 이전에 HDR 공간에서 합산해야 한다.
//  ToneMapPass가 이 RT를 읽어야 하므로 mAfter = ToneMap 입력 RT로 설정.
//
//  PassCustomTable[POST_BLOOM_COMP_PASS]:
//   PreviousStep   : HDR SceneColor GBuffer 인덱스
//   ExtValue[0].x  : bloomIntensity — 최종 bloom 합산 강도 (권장 1.0)
// ─────────────────────────────────────────────────────────────────────────────

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float bloomIntensity = data.ExtValue[0].x;

    // HDR SceneColor (에미시브 포함, 톤맵 전)
    float3 hdr = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv).rgb;

    // Bloom (피라미드 업샘플 누적 완료, 1/2 해상도 → bilinear로 fullscreen 합산)
    float3 bloom = BloomMips[0].Sample(g_sam_0, input.uv).rgb;

    float3 result = hdr + bloom * bloomIntensity;

    return float4(result, 1.0f);
}
