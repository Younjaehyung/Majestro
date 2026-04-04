#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bloom Upsample Pass  ── 3×3 tent filter (additive blend로 등록할 것)
//
//  [원칙]
//   이 쉐이더는 ONE_TO_ONE_BLEND (src=1, dst=1)으로 렌더링된다.
//   → 이미 downsampled 데이터가 들어있는 RT 위에 upsampled 기여분이 누적됨.
//   → Clear 없이 OMSetRenderTargets 호출 필수 (BloomPass::Execute 참조).
//
//  3×3 tent filter 가중치:
//    1 2 1
//    2 4 2  × (1/16)
//    1 2 1
//
//  PassCustomTable[POST_BLOOM_US_PASS]:
//   ExtTex[0]       : 소스 BloomMips 인덱스 (N+1 레벨, 즉 더 coarse한 mip)
//   ExtValue[0].x   : srcTexelW = 1.0 / 소스 RT 너비
//   ExtValue[0].y   : srcTexelH = 1.0 / 소스 RT 높이
//   ExtValue[0].z   : upsampleStrength — 업샘플 기여 배율 (권장 0.85~1.0)
// ─────────────────────────────────────────────────────────────────────────────

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex]; // upsampleStrength 읽기용

    // srcMip: GlobalParams.etc (3=MIP3→MIP2, 2=MIP2→MIP1, 1=MIP1→MIP0)
    uint  srcMip   = GlobalParams.etc;
    float strength = data.ExtValue[0].z;     // upsampleStrength
    float2 uv      = input.uv;

    // 소스(coarser) 해상도 텍셀 크기
    float srcScale = (float)(1u << (srcMip + 1u));
    float2 ts = float2(srcScale / PassParams.ScreenSize.x,
                       srcScale / PassParams.ScreenSize.y);

    // 3×3 tent filter
    float3 sum = float3(0, 0, 0);
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2(-1,-1) * ts).rgb * 1.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2( 0,-1) * ts).rgb * 2.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2( 1,-1) * ts).rgb * 1.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2(-1, 0) * ts).rgb * 2.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv                     ).rgb * 4.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2( 1, 0) * ts).rgb * 2.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2(-1, 1) * ts).rgb * 1.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2( 0, 1) * ts).rgb * 2.0f;
    sum += BloomMips[srcMip].Sample(g_sam_0, uv + float2( 1, 1) * ts).rgb * 1.0f;

    float3 upsampled = (sum / 16.0f) * strength;

    // additive blend (DST += upsampled)이므로 SV_Target에 upsampled만 출력
    return float4(max(upsampled, 0.0f), 1.0f);
}
