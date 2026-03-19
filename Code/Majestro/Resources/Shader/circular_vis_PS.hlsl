
#include "params.hlsl"

// circular_vis_PS.hlsl
// 원형 오디오 비주얼라이저 ribbon 픽셀 셰이더.
//
// 입력:
//   intensity = 해당 위치의 진폭 (0 이상, 스파이크 시 > 1)
//
// 색상 규칙:
//   - 흰색 단색 기본
//   - intensity가 낮을수록 알파가 줄어들어 얇은 구간이 자연스럽게 페이드됨
//   - 알파 블렌딩이 PSO에서 활성화되어 있어야 동작함 (ALPHA_BLEND 설정)

struct VS_OUT
{
    float4 pos       : SV_POSITION;
    float  intensity : TEXCOORD0;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    // intensity 기반 edge fade: 진폭이 0에 가까울수록 투명해져
    // 소리 없는 구간은 얇은 반투명 선으로, 강한 구간은 불투명한 띠로 표현됨
    float edge = saturate(input.intensity * 8.0f);

    return float4(1.0f, 1.0f, 1.0f, edge);
}
