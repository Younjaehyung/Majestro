#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_CONQUEST_PARAMS> GlobalParams : register(b0, space0);
// 점령 진행률 원형 게이지 전용 PS.
// 슬롯 재사용 매핑 (GLOBAL_PARAMS 와 일대일):
//   Progress               :  점령 진행률
//   BackgroundTextureIndex :  채워지지 않은 링 텍스처
//   FillTextureIndex       :  채워진 링 텍스처
//   InnerRadiusEncoded     :  내부 반지름 (0~1000)
//   PassFlags              :  첫 번째 비트는 HUD 모드, 두 번째 비트는 완료 글로우
//   AlphaEncoded           :  알파 (0~1000)
struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    const float progress = saturate(GlobalParams.Progress);
    const float innerRadius = saturate(float(GlobalParams.InnerRadiusEncoded) * 0.001f);

    // (0,0) ~ (1,1) → 중심 기준 (-1..1)
    float2 d = input.uv - 0.5f;
    float r = length(d) * 2.0f;

    // 도넛 마스크 (외곽 / 내부 컷)
    if (r > 1.0f) discard;
    if (r < innerRadius) discard;

    // 12시 = 0, 시계방향 증가
    float angle = atan2(d.x, -d.y);
    if (angle < 0.0f) angle += 6.2831853f;

    const float fillEnd = progress * 6.2831853f;
    const bool isFilled = angle <= fillEnd;

    uint texIdx = isFilled ? GlobalParams.FillTextureIndex : GlobalParams.BackgroundTextureIndex;
    float4 srcColor = TextureMaps[texIdx].Sample(g_sam_0, input.uv);

    // 알파 (0 = 불투명). 룰렛 슬롯의 좌우 페이드아웃과 완료 플래시가 함께 사용
    if (GlobalParams.AlphaEncoded != 0)
        srcColor.a *= saturate(float(GlobalParams.AlphaEncoded) * 0.001f);

    // 글로우는 알파와 분리 — 완료 플래시일 때만 켠다.
    if ((GlobalParams.PassFlags & 2) != 0)
        srcColor.rgb *= 1.5f;

    if (srcColor.a < 0.05f)
        discard;

    // World 모드는 depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.PassFlags & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    return srcColor;
}
