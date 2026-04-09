#include "params.hlsl"

// ──────────────────────────────────────────────────────────────
// Emissive Bloom Composite Pass
//
// PassCustomTable[POST_KAWASE_COMPOSITE_PASS = 15]:
//   PreviousStep  : mBefore RT GBuffer 인덱스 (씬 컬러)
//   ExtTex[0]     : 최종 업샘플 RT의 TextureMaps 이미지 인덱스
//   ExtValue[0].x : intensity (합성 강도, 기본 0.8)
//
// 출력: scene + emissiveBloom * intensity (HDR 공간 덧셈)
// ──────────────────────────────────────────────────────────────

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float4 scene    = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv);

    int    emissiveIdx = data.ExtTex[0];
    float  intensity   = data.ExtValue[0].x;

    float4 bloom = TextureMaps[emissiveIdx].Sample(g_sam_0, input.uv);

    // HDR 공간 가산 합성
    float3 result = scene.rgb + bloom.rgb * intensity;

    return float4(result, scene.a);
}
