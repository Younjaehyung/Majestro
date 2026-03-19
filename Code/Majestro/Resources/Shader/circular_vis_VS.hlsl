
#include "params.hlsl"

// circular_vis_VS.hlsl
// 원형 오디오 비주얼라이저 ribbon 버텍스 셰이더.
// 버텍스 버퍼에서 직접 픽셀 좌표를 읽어 NDC로 변환한다.
//
// 버텍스 레이아웃:
//   POSITION.xy = 화면 픽셀 좌표
//   POSITION.z  = 미사용 (0)
//   TEXCOORD.x  = intensity (진폭 0 이상, 스파이크 시 > 1)
//   TEXCOORD.y  = 미사용 (0)

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv  : TEXCOORD;
};

struct VS_OUT
{
    float4 pos       : SV_POSITION;
    float  intensity : TEXCOORD0;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    // 픽셀 좌표 → NDC 변환 (좌상단=(-1,1), 우하단=(1,-1))
    float2 ndc;
    ndc.x =  (input.pos.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = -(input.pos.y / PassParams.ScreenSize.y) * 2.0f + 1.0f;

    output.pos       = float4(ndc, 0.0f, 1.0f);
    output.intensity = input.uv.x;

    return output;
}
