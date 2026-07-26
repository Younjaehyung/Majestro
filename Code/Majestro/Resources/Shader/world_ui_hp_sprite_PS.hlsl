#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_SPRITE_PARAMS> GlobalParams : register(b0, space0);

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1;
};

// HP 바 배경/채움 전용 PS.
float4 PS_Main(VS_OUT input) : SV_Target
{
    const uint role = GlobalParams.SpriteRole; // 0=배경, 1=채움
    //     0 : 배경 텍스처
    //     1 : 채움 텍스처를 사용하고 Progress 이후 영역을 버린다
    
    // 채움이면 followRatio 기준으로 UV 우측 잘라내기
    if (role == 1 && input.uv.x > GlobalParams.Progress)
        discard;

    uint texIdx = (role == 1) ? GlobalParams.FillTextureIndex : GlobalParams.BackgroundTextureIndex;

    float4 srcColor = TextureMaps[texIdx].Sample(g_sam_0, input.uv);

    if (srcColor.a < 0.05f)
        discard;

    // Depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.PassFlags & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    return srcColor;
}
