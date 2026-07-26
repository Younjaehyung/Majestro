
#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_HP_EFFECT_PARAMS> GlobalParams : register(b0, space0);
// HP 바 hit effect 전용 VS (WorldUIPass).
// 인스턴스마다 UIInstanceData에 다음을 패킹:
//   Position(xy) = 앵커 기준 픽셀 오프셋 (피격 순간 남은 HP 우측 끝 고정)
//   Size(xy)     = 스프라이트 픽셀 크기 (화면 픽셀 단위, HP 바와 독립)
//   Pivot.x      = alpha (1.f - Age/LifeTime, 페이드아웃)
//   Pivot.y      = ageRatio (Age/LifeTime, 0~1) — 시트 프레임 인덱스 계산용

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 sheetUV    : TEXCOORD0; // 시트 셀 내부 UV
    float  alpha      : VALPHA;
    float  anchorZNDC : TEXCOORD1;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    UIInstanceData inst = UIInstances[idx];

    float2 anchorPx = inst.Position;
    float2 sizePx   = inst.Size;
    float  alpha    = inst.Pivot.x;
    float  ageRatio = saturate(inst.Pivot.y);

    // 앵커 ->NDC (World vs HUD)
    // NDC.z = 앵커 깊이 (PS occlusion 비교용)
    const bool isHud = (GlobalParams.PassFlags & 1u) != 0u;
    float2 anchorNDC;
    float anchorZNDC;
    float anchorW;
    if (isHud)
    {
        float2 px = GlobalParams.Anchor.xy;
        anchorNDC.x =  (px.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
        anchorNDC.y = -(px.y / PassParams.ScreenSize.y) * 2.0f + 1.0f;
        anchorZNDC = 0.0f;
        anchorW = 1.0f;
    }
    else
    {
        float4 anchorClip = mul(float4(GlobalParams.Anchor, 1.0f), PassParams.MatView);
        anchorClip = mul(anchorClip, PassParams.MatProjection);
        if (anchorClip.w <= 0.0f)
        {
            output.pos = float4(2.0f, 2.0f, 0.0f, 1.0f);
            output.sheetUV = float2(0, 0);
            output.alpha = 0;
            output.anchorZNDC = 1;
            return output;
        }
        anchorNDC = anchorClip.xy / anchorClip.w;
        anchorZNDC = anchorClip.z / anchorClip.w;
        anchorW = anchorClip.w;
    }

    //  픽셀 오프셋
    float2 quadOffsetPx = (input.pos.xy - 0.5f) * sizePx;
    float2 totalOffsetPx = anchorPx + quadOffsetPx;

    // NDC 오프셋
    float2 ndcOffset;
    ndcOffset.x =  (totalOffsetPx.x / PassParams.ScreenSize.x) * 2.0f;
    ndcOffset.y = -(totalOffsetPx.y / PassParams.ScreenSize.y) * 2.0f;
    float2 finalNDC = anchorNDC + ndcOffset;

    output.pos = float4(finalNDC * anchorW, anchorZNDC * anchorW, anchorW);

    // Sheet Sprite 프레임 UV 계산
    uint cols       = GlobalParams.HitConfig & 0xFFu;
    uint rows       = (GlobalParams.HitConfig >> 8u) & 0xFFu;
    uint frameCount = (GlobalParams.HitConfig >> 16u) & 0xFFFFu;
    cols       = max(cols, 1u);
    rows       = max(rows, 1u);
    frameCount = max(frameCount, 1u);

    uint frameIdx = (uint)(ageRatio * (float)frameCount);
    frameIdx = min(frameIdx, frameCount - 1u);
    uint col = frameIdx % cols;
    uint row = frameIdx / cols;

    float2 cellSize = float2(1.0f / (float)cols, 1.0f / (float)rows);
    output.sheetUV = (float2(col, row) + input.uv) * cellSize;

    output.alpha = alpha;
    output.anchorZNDC = anchorZNDC;

    return output;
}
