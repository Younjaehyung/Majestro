
#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_HP_EFFECT_PARAMS> GlobalParams : register(b0, space0);

// barycentric + fwidth로 삼각형 경계(엣지)를 검정으로 강조
// VS에서 받은 alpha을 곱해 페이드아웃
// PreDepth 비교로 occluded 영역은 discard

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float3 bary       : BARYCENTRIC;
    float2 spriteUV   : TEXCOORD0;
    float  alpha      : VALPHA;
    float  anchorZNDC : TEXCOORD1;
};

float EdgeFactor(float3 b, float widthPx)
{
    float3 d = fwidth(b);
    float3 s = smoothstep(float3(0.0f, 0.0f, 0.0f), d * widthPx, b);
    return min(min(s.x, s.y), s.z);
}

// HP 손실 파편 전용 PS (WorldUIPass).
float4 PS_Main(VS_OUT input) : SV_Target
{
    // sprite 모양으로 마스킹
    if (any(input.spriteUV < 0.0f) || any(input.spriteUV > 1.0f))
        discard;
    // alpha는 모양 마스크
    float4 fillTexel = TextureMaps[GlobalParams.FillTextureIndex].Sample(g_sam_0, input.spriteUV);
    if (fillTexel.a < 0.05f)
        discard;

    // Depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.PassFlags & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    // 빨강 + 검정 엣지
    const float3 fillColor = float3(1.0f, 0.08f, 0.08f);
    const float3 edgeColor = float3(0.0f, 0.0f, 0.0f);
    const float  edgeWidth = 1.2f;

    float e = EdgeFactor(input.bary, edgeWidth);
    float3 rgb = lerp(edgeColor, fillColor, e);

    float a = input.alpha;
    if (a < 0.01f)
        discard;

    return float4(rgb, a);
}
