
#include "params.hlsl"
#include "utils.hlsl"


// - 내부: 빨강
// - 삼각형 경계(엣지): 검정 (barycentric + fwidth 기반 wireframe)
// - 알파: VS에서 전달받은 값(파편 수명 진행도 기반)

struct VS_OUT
{
    float4 pos   : SV_POSITION;
    float3 bary  : BARYCENTRIC;
    float  alpha : VALPHA;
};

// barycentric이 0에 가까울수록(= 엣지) 0, 내부일수록 1
float EdgeFactor(float3 b, float widthPx)
{
    float3 d = fwidth(b);
    float3 s = smoothstep(float3(0.0f, 0.0f, 0.0f), d * widthPx, b);
    return min(min(s.x, s.y), s.z);
}

// HP 손실 파편 전용 PS
float4 PS_Main(VS_OUT input) : SV_Target
{
    const float3 fillColor = float3(1.0f, 0.08f, 0.08f); // 빨강
    const float3 edgeColor = float3(0.0f, 0.0f, 0.0f);   // 검정
    const float  edgeWidth = 1.2f;

    float e = EdgeFactor(input.bary, edgeWidth); 
    float3 rgb = lerp(edgeColor, fillColor, e);

    float a = input.alpha;
    if (a < 0.01f)
        discard;

    return float4(rgb, a);
}
