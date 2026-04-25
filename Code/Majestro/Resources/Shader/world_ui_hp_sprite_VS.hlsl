
#include "params.hlsl"
#include "utils.hlsl"



// World 모드:
//  UIQuad의 vertex.xy(0~1)에 픽셀 사이즈를 곱해 HpBarPivotPx만큼 이동
//  NDC.xy에 픽셀 오프셋(스크린크기 정규화)을 더하면 거리 무관 일정 픽셀 크기 빌보드
//  NDC.z = 앵커 깊이 (PS에서 Gbuffer와 비교용)
// HUD 모드 (etc bit0 = 1):
//   HpBarAnchorWorld.xy 를 화면 픽셀로 직접 해석


struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VS_OUT
{
    float4 pos       : SV_POSITION;
    float2 uv        : TEXCOORD0;
    float  anchorZNDC : TEXCOORD1; // PS에서 depth 비교용
};

// HP 바 배경/채움 sprite 전용 VS.
VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    const bool isHud = (GlobalParams.etc & 1) != 0;

    float2 anchorNDC;
    float anchorZNDC;
    float anchorW;

    if (isHud)
    {
        float2 px = GlobalParams.HpBarAnchorWorld.xy;
        anchorNDC.x =  (px.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
        anchorNDC.y = -(px.y / PassParams.ScreenSize.y) * 2.0f + 1.0f;
        anchorZNDC = 0.0f;
        anchorW = 1.0f;
    }
    else
    {
        float4 anchorClip = mul(float4(GlobalParams.HpBarAnchorWorld, 1.0f), PassParams.MatView);
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


    float2 pixelOffset = GlobalParams.HpBarPivotPx + input.pos.xy * GlobalParams.HpBarSizePx;

    // 스크린 크기로 정규화, w 무관 = 거리 무관
    float2 ndcOffset;
    ndcOffset.x =  (pixelOffset.x / PassParams.ScreenSize.x) * 2.0f;
    ndcOffset.y = -(pixelOffset.y / PassParams.ScreenSize.y) * 2.0f;

    float2 finalNDC = anchorNDC + ndcOffset;

    // HUD 면 anchorW=1, anchorZNDC=0, output = (finalNDC, 0, 1)
    output.pos = float4(finalNDC * anchorW, anchorZNDC * anchorW, anchorW);
    output.uv = input.uv;
    output.anchorZNDC = anchorZNDC;

    return output;
}
