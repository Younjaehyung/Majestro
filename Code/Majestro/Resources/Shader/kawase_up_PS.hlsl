#include "params.hlsl"

// ──────────────────────────────────────────────────────────────
// Dual Kawase Upsample Pass
//
// PassCustomTable[POST_KAWASE_UP_PASS = 14]:
//   ExtValue[step] = (texelW, texelH, float(srcImageIdx), 0)
//
// GlobalParams.etc = 현재 step 인덱스 (0~N-1)
//
// Dual Kawase Upsample 커널 (8-tap):
//   +─────────────────────────────────────────────+
//   │  ×1   ×2   ×1                               │
//   │  ×2        ×2    (가운데는 샘플 없음)         │
//   │  ×1   ×2   ×1    -> 총 12로 나눔              │
//   +─────────────────────────────────────────────+
//   오프셋 = halfTexel (소스 텍스처 기준 절반 텍셀)
// ──────────────────────────────────────────────────────────────

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint step = GlobalParams.etc;

    float texelW   = data.ExtValue[step].x;
    float texelH   = data.ExtValue[step].y;
    uint  srcIdx   = (uint) data.ExtValue[step].z;

    float2 uv        = input.uv;
    float2 halfTexel = float2(texelW, texelH) * 0.5f;

    // Dual Kawase Upsample 8-tap
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 가중치 배치 (총합 12):
    //  좌(-2,0) ×1,  좌상(-1,+1) ×2,  상(0,+2) ×1,  우상(+1,+1) ×2
    //  우(+2,0) ×1,  우하(+1,-1) ×2,  하(0,-2) ×1,  좌하(-1,-1) ×2
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(-2.0f * halfTexel.x,              0.0f))) * 1.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(       -halfTexel.x,  +halfTexel.y))) * 2.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(              0.0f, +2.0f * halfTexel.y))) * 1.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(       +halfTexel.x,  +halfTexel.y))) * 2.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(+2.0f * halfTexel.x,              0.0f))) * 1.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(       +halfTexel.x,  -halfTexel.y))) * 2.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(              0.0f, -2.0f * halfTexel.y))) * 1.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(       -halfTexel.x,  -halfTexel.y))) * 2.0f;

    color /= 12.0f;

    return color;
}
