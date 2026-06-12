
#include "params.hlsl"

// circular_vis_PS.hlsl
// 원형 오디오 비주얼라이저 막대 픽셀 셰이더.
//
// 입력:
//   intensity = 해당 막대의 진폭 (0~1)
//
// 색상 규칙 (미니멀 스타일):
//   - 흰색(약간 회색기) 단색
//   - 무음(intensity=0)이어도 기본 알파를 유지해 minBarLength 막대들이
//     점선 형태의 원형 가이드 링으로 보인다
//   - 진폭이 클수록 불투명해져 활성 막대가 또렷해짐
//   - 알파 블렌딩이 PSO에서 활성화되어 있어야 동작함 (ALPHA_BLEND 설정)

struct VS_OUT
{
    float4 pos       : SV_POSITION;
    float  intensity : TEXCOORD0;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    float t = saturate(input.intensity);

    // 무음: 0.35 (은은한 링) | 최대 진폭: 0.95 (또렷한 막대)
    float alpha = lerp(0.35f, 0.95f, t);

    return float4(0.92f, 0.92f, 0.92f, alpha);
}
