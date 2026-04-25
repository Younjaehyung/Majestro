#include "params.hlsl"
#include "utils.hlsl"


struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1;
};

// HP 바 배경/채움 전용 PS.
float4 PS_Main(VS_OUT input) : SV_Target
{
    const uint role = GlobalParams.casdcae; // 0=배경, 1=채움
    //     0 = 배경 (HpBarBgTexIdx 사용, 풀 폭)
    //     1 = 채움 (HpBarFillTexIdx 사용, uv.x > followRatio면 discard)
    
    // 채움이면 followRatio 기준으로 UV 우측 잘라내기
    if (role == 1 && input.uv.x > GlobalParams.HpBarFollowRatio)
        discard;

    uint texIdx = (role == 1) ? GlobalParams.HpBarFillTexIdx  : GlobalParams.HpBarBgTexIdx;

    float4 srcColor = TextureMaps[texIdx].Sample(g_sam_0, input.uv);

    if (srcColor.a < 0.05f)
        discard;

    // Depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.etc & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    return srcColor;
}
