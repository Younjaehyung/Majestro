
#include "world_ui_params.hlsl"
#include "utils.hlsl"

ConstantBuffer<WORLD_UI_HP_EFFECT_PARAMS> GlobalParams : register(b0, space0);

// 인스턴스마다 UIInstanceData에 다음을 패킹:
//   Position(xy) = V0 픽셀 오프셋 (앵커 기준)
//   Size(xy)     = V1 픽셀 오프셋
//   Pivot(xy)    = V2 픽셀 오프셋
//   ZOrder       = 파편 알파 (0~1)


//  배경 sprite와 동일한 UV
//  UV는 정점 픽셀 오프셋과 PivotPx 및 SizePx를 사용해 계산한다
//  barycentric은 각 vertex의 입력 uv가 그대로 가중치 분배 ((1,0,0)/(0,1,0)/(0,0,1))


struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float3 bary       : BARYCENTRIC;
    float2 spriteUV   : TEXCOORD0; // 배경 sprite 알파 마스킹용
    float  alpha      : VALPHA;
    float  anchorZNDC : TEXCOORD1;
};

// HP 손실 파편 전용 VS (WorldUIPass).
VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    UIInstanceData inst = UIInstances[idx];

    // 입력 UV로 V0/V1/V2 가중치 분배
    float w0 = (1.0f - input.uv.x) * (1.0f - input.uv.y);
    float w1 = input.uv.x * (1.0f - input.uv.y);
    float w2 = input.uv.x * input.uv.y;

    // 앵커 기준 픽셀 오프셋
    float2 offsetPx = inst.Position * w0 + inst.Size * w1 + inst.Pivot * w2;

    // NDC 계산 (World vs HUD)
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
    { // 거리 무관 빌보드
      // NDC에 픽셀 오프셋(스크린 정규화) 더함
        float4 anchorClip = mul(float4(GlobalParams.Anchor, 1.0f), PassParams.MatView);
        anchorClip = mul(anchorClip, PassParams.MatProjection);
        if (anchorClip.w <= 0.0f)
        {
            output.pos = float4(2.0f, 2.0f, 0.0f, 1.0f);
            output.bary = float3(w0, w1, w2);
            output.spriteUV = float2(0, 0);
            output.alpha = 0;
            output.anchorZNDC = 1;
            return output;
        }
        anchorNDC = anchorClip.xy / anchorClip.w;
        anchorZNDC = anchorClip.z / anchorClip.w;
        anchorW = anchorClip.w;
    }

    //  NDC 오프셋
    float2 ndcOffset;
    ndcOffset.x =  (offsetPx.x / PassParams.ScreenSize.x) * 2.0f;
    ndcOffset.y = -(offsetPx.y / PassParams.ScreenSize.y) * 2.0f;
    float2 finalNDC = anchorNDC + ndcOffset;

    output.pos = float4(finalNDC * anchorW, anchorZNDC * anchorW, anchorW);
    output.bary = float3(w0, w1, w2);

    // 배경 sprite와 동일한 UV: (오프셋 - 바좌상단) / 바크기
    output.spriteUV = (offsetPx - GlobalParams.PivotPx) / GlobalParams.SizePx;

    output.alpha = inst.ZOrder;
    output.anchorZNDC = anchorZNDC;

    return output;
}
