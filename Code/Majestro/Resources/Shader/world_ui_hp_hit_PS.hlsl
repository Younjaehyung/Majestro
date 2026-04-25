
#include "params.hlsl"
#include "utils.hlsl"


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
    if (GlobalParams.HpBarHitTexIdx == 0u)
        discard;

    // Depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.etc & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    float4 srcColor = TextureMaps[GlobalParams.HpBarHitTexIdx].Sample(g_sam_0, input.sheetUV);

    float a = srcColor.a * input.alpha;
    if (a < 0.01f)
        discard;

    return float4(srcColor.rgb * a, a);
}
