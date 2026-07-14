#include "params.hlsl"

// ──────────────────────────────────────────────────────────────
// Dual Kawase Downsample Pass
//
// PassCustomTable[POST_KAWASE_DOWN_PASS = 13]:
//   ExtValue[step] = (texelW, texelH, float(srcImageIdx), 0)
//     texelW/H   : 소스 텍스처의 텍셀 크기 (1 / srcWidth, 1 / srcHeight)
//     srcImageIdx: 소스 텍스처의 TextureMaps[] 인덱스 (GetImageIndex() 값)
//
// GlobalParams.PassScalar0 = 현재 step 인덱스 (0~N-1)
//   -> ExtValue[etc]에서 해당 step의 params 조회
//
// Dual Kawase Downsample 커널 (5-tap):
//   center × 4 + 대각선 4방향 샘플 (halfTexel 오프셋) -> / 8
// ──────────────────────────────────────────────────────────────

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint step = GlobalParams.PassScalar0;

    float texelW    = data.ExtValue[step].x;
    float texelH    = data.ExtValue[step].y;
    uint  srcIdx    = (uint) data.ExtValue[step].z;

    float2 uv         = input.uv;
    float2 halfTexel  = float2(texelW, texelH) * 0.5f;

    // Dual Kawase Downsample: 중심 픽셀(가중치 4) + 4 대각선(각 가중치 1) -> 총 8로 나눔
    // UV 클램프: 스크린 경계에서 WRAP 아티팩트 방지
    float4 color;
    color  = TextureMaps[srcIdx].Sample(g_sam_0, uv) * 4.0f;
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(+halfTexel.x, +halfTexel.y)));
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(-halfTexel.x, +halfTexel.y)));
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(+halfTexel.x, -halfTexel.y)));
    color += TextureMaps[srcIdx].Sample(g_sam_0,
                saturate(uv + float2(-halfTexel.x, -halfTexel.y)));
    color /= 8.0f;

    return color;
}
