
#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_HP_EFFECT_PARAMS> GlobalParams : register(b0, space0);

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 sheetUV    : TEXCOORD0;
    float  alpha      : VALPHA;
    float  anchorZNDC : TEXCOORD1;
};

// HP 바 hit effect 전용 PS (WorldUIPass).
float4 PS_Main(VS_OUT input) : SV_Target
{
    if (GlobalParams.HitTextureIndex == 0u)
        discard;

    // Depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.PassFlags & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    float4 srcColor = TextureMaps[GlobalParams.HitTextureIndex].Sample(g_sam_0, input.sheetUV);

    float a = srcColor.a * input.alpha;
    if (a < 0.01f)
        discard;

    return float4(srcColor.rgb * a, a);
}
