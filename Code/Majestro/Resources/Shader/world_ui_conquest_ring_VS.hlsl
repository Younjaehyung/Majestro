
#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_CONQUEST_PARAMS> GlobalParams : register(b0, space0);
// 점령 진행률 원형 게이지
// world_ui_hp_sprite_VS.hlsl 와 동일한 앵커/오프셋 변환 (HUD/World 양쪽 지원).
//   HUD   :  Anchor.xy를 화면 픽셀로 직접 사용
//   World : Anchor.xyz를 월드 좌표로 사용
// quad의 (uv) 는 PS 에서 (0..1) 로 받아 라디얼 필 / 도넛 마스크 계산에 사용.

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    const bool isHud = (GlobalParams.PassFlags & 1) != 0;

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
            output.uv = input.uv;
            output.anchorZNDC = 1.0f;
            return output;
        }
        anchorNDC = anchorClip.xy / anchorClip.w;
        anchorZNDC = anchorClip.z / anchorClip.w;
        anchorW = anchorClip.w;
    }

    float2 pixelOffset = GlobalParams.PivotPx + input.pos.xy * GlobalParams.SizePx;

    float2 ndcOffset;
    ndcOffset.x =  (pixelOffset.x / PassParams.ScreenSize.x) * 2.0f;
    ndcOffset.y = -(pixelOffset.y / PassParams.ScreenSize.y) * 2.0f;

    float2 finalNDC = anchorNDC + ndcOffset;

    output.pos = float4(finalNDC * anchorW, anchorZNDC * anchorW, anchorW);
    output.uv = input.uv;
    output.anchorZNDC = anchorZNDC;

    return output;
}
