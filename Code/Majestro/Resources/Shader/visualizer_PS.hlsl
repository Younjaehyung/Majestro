
#include "params.hlsl"
#include "utils.hlsl"

// visualizer_PS.hlsl
// AudioVisualizer 전용 픽셀 셰이더. VS는 visualizer_VS.hlsl (BaseInstanceID 오프셋 가산).
//
// UIInstanceData 필드 재해석:
//   MaterialIndex — 바 인덱스 (0~31), 주파수 그라데이션 색상 결정
//   ZOrder        — 정규화 바 높이 (0~1), 글로우 강도 계산

struct VS_OUT
{
    float4 pos        : SV_POSITION;
    float2 uv         : TEXCOORD;
    uint   instanceID : InstanceID;
};

// UI_VS.hlsl의 Pivot=(0.5, 1) 설정 결과:
//   uv.y = 0  →  바의 꼭대기 (화면 위쪽)
//   uv.y = 1  →  바의 바닥   (화면 아래쪽)

float4 PS_Main(VS_OUT input) : SV_Target
{
    UIInstanceData inst = UIInstances[input.instanceID];

    int   barIdx  = (int) inst.MaterialIndex;   // 0 ~ 31 (VISUALIZER_BAR_COUNT - 1)
    float normH   = inst.ZOrder;                // 0~1: 현재 바 높이 비율
    float uvY     = input.uv.y;                 // 0=꼭대기, 1=바닥

    // 주파수 T: 0 = 저음(베이스), 1 = 고음(트레블)
    float freqT = saturate((float) barIdx / 31.0f);

    // ----- 주파수별 기본 색상 (저음=주황, 중음=보라, 고음=하늘) -----
    float3 lowColor  = float3(1.00f, 0.35f, 0.05f);
    float3 midColor  = float3(0.75f, 0.10f, 0.90f);
    float3 highColor = float3(0.05f, 0.65f, 1.00f);

    float3 baseColor;
    if (freqT < 0.7f)
        baseColor = lerp(lowColor,  midColor,  freqT * 2.0f);
    else
        baseColor = lerp(midColor, highColor, (freqT - 0.5f) * 2.0f);

    // ----- 높이 방향 그라데이션 -----
    // uvY=0(꼭대기) 에서 밝게, uvY=1(바닥)에서 어둡게
    float heightGrad = 1.0f - uvY;   // 0=바닥, 1=꼭대기

    // 꼭대기 발광: 마지막 10% 구간에서 흰색으로 과포화
    float glowMask  = smoothstep(0.85f, 1.0f, heightGrad);
    float3 color    = lerp(baseColor * 0.35f, baseColor * 1.5f, heightGrad);
    color           = lerp(color, float3(1.5f, 1.5f, 1.5f), glowMask * normH);

    // ----- 투명도 -----
    // 전체 바 높이가 낮을수록 불투명도 감소 (무음 시 자연스럽게 사라짐)
    float alpha = lerp(0.0f, 0.92f, normH);
    // 바닥 쪽으로 갈수록 약간 페이드
    alpha *= lerp(0.6f, 1.0f, heightGrad);

    // 높이가 0에 가까우면 완전히 버림 (1px 쓰레기 드로우 방지)
    if (normH < 0.005f)
        discard;

    return float4(color, alpha);
}
