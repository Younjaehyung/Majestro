#include "params.hlsl"
#include "utils.hlsl"

// 점령 진행률 원형 게이지 전용 PS.
// 슬롯 재사용 매핑 (GLOBAL_PARAMS 와 일대일):
//   HpBarFollowRatio  -> progress (0~1)
//   HpBarBgTexIdx     -> 링 배경 텍스처 인덱스 (안 채워진 부분)
//   HpBarFillTexIdx   -> 링 채움 텍스처 인덱스 (채워진 부분)
//   casdcae           -> 도넛 내부 반지름 ([0..1000] 정수로 인코딩, /1000.0 = innerRadius)
//   etc bit0          -> HUD 모드 (1) / World 모드 (0)

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    const float progress = saturate(GlobalParams.HpBarFollowRatio);
    const float innerRadius = saturate(float(GlobalParams.casdcae) * 0.001f);

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

    uint texIdx = isFilled ? GlobalParams.HpBarFillTexIdx : GlobalParams.HpBarBgTexIdx;
    float4 srcColor = TextureMaps[texIdx].Sample(g_sam_0, input.uv);

    if (srcColor.a < 0.05f)
        discard;

    // World 모드는 depth occlusion (HUD 모드는 스킵)
    if ((GlobalParams.etc & 1) == 0)
    {
        float2 screenUV = input.pos.xy / PassParams.ScreenSize;
        float sceneDepth = Gbuffer[0].SampleLevel(g_sam_0, screenUV, 0).r;
        if (sceneDepth < input.anchorZNDC - 1e-5f)
            discard;
    }

    return srcColor;
}
