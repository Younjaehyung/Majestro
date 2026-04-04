#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bloom Kawase Blur Pass
//   대각선 4방향 샘플링으로 넓은 글로우를 만드는 Kawase 블러
//   여러 번 반복할수록 더 넓고 부드러운 블룸이 만들어짐
//
// GlobalParams.etc      : 소스 Gbuffer 인덱스 (BLOOM_A 또는 BLOOM_B)
// GlobalParams.casdcae  : Kawase 반복 횟수 (offset = casdcae + 0.5)
// ─────────────────────────────────────────────────────────────────────────────

float4 PS_Main(VS_OUT input) : SV_Target
{
    float2 texelSize = 1.0f / float2(PassParams.ScreenSize.x, PassParams.ScreenSize.y);
    float  offset    = (float)GlobalParams.casdcae + 0.5f;
    uint   srcIdx    = GlobalParams.etc;

    float2 uv = input.uv;
    float3 sum = 0.f;
    sum += Gbuffer[srcIdx].Sample(g_sam_0, uv + float2(-offset, -offset) * texelSize).rgb;
    sum += Gbuffer[srcIdx].Sample(g_sam_0, uv + float2( offset, -offset) * texelSize).rgb;
    sum += Gbuffer[srcIdx].Sample(g_sam_0, uv + float2(-offset,  offset) * texelSize).rgb;
    sum += Gbuffer[srcIdx].Sample(g_sam_0, uv + float2( offset,  offset) * texelSize).rgb;

    return float4(sum * 0.25f, 1.f);
}
