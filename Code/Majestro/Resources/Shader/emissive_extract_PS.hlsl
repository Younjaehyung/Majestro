#include "params.hlsl"

// ──────────────────────────────────────────────────────────────
// Emissive Extract Pass
//
// PassCustomTable[POST_KAWASE_EXTRACT_PASS = 12]:
//   ExtValue[0].x : 이미시브 강도 스케일 (기본 1.0, 0 이하이면 1.0 사용)
//
// Gbuffer[4] = GBUFFER_EMISSIVE_INDEX
//
// ──────────────────────────────────────────────────────────────

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    // 이미시브 GBuffer RT 직접 참조 (인덱스 4 = GBUFFER_EMISSIVE_INDEX)
    float3 emissive = Gbuffer[4].Sample(g_sam_0, input.uv).rgb;

    
    // 선택적 강도 스케일 (ExtValue[0].x, 기본 1.0)
    float scale = (data.ExtValue[0].x > 0.0f) ? data.ExtValue[0].x : 1.0f;
    
    
    return float4(emissive * scale, 1.0f);
}
