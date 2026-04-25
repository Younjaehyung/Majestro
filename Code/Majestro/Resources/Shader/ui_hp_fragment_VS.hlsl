
#include "params.hlsl"
#include "utils.hlsl"


// UIInstanceData 필드 해석:
//   Position(float2) = 삼각형 V0 픽셀 좌표
//   Size(float2)     = 삼각형 V1 픽셀 좌표
//   Pivot(float2)    = 삼각형 V2 픽셀 좌표
//   ZOrder(float)    = 파편 알파 (0~1)


//
// UITriangle 메시는 3정점(3인덱스):
//   vertex[0] pos=(0,0) uv=(0,0) -> V0 (barycentric (1,0,0))
//   vertex[1] pos=(1,0) uv=(1,0) -> V1 (barycentric (0,1,0))
//   vertex[2] pos=(1,1) uv=(1,1) -> V2 (barycentric (0,0,1))

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos   : SV_POSITION;
    float3 bary  : BARYCENTRIC;
    float  alpha : VALPHA;
};

// HP 손실 파편 전용 VS
VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    UIInstanceData inst = UIInstances[idx];

    // UV로 V0/V1/V2 가중치 분배 (3정점에서 정확히 한 가중치만 1)
    float w0 = (1.0f - input.uv.x) * (1.0f - input.uv.y); // (0,0) -> 1
    float w1 = input.uv.x * (1.0f - input.uv.y);          // (1,0) -> 1
    float w2 = input.uv.x * input.uv.y;                   // (1,1) -> 1

    float2 pixel = inst.Position * w0 + inst.Size * w1 + inst.Pivot * w2;

    float2 ndc;
    ndc.x = (pixel.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (pixel.y / PassParams.ScreenSize.y) * 2.0f;

    output.pos = float4(ndc, 0.0f, 1.0f);
    output.bary = float3(w0, w1, w2);
    output.alpha = inst.ZOrder;

    return output;
}
