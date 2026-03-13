#include "params.hlsl"

struct VS_OUT
{
    float4 pos                      : SV_Position;
    nointerpolation uint matIdx     : TEXCOORD0;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    MATERIALINFO mat = Materials[input.matIdx]; 
    // 아웃라인 정보는 PassCustomData의 ExtValue[0]에 저장되어 있다고 가정

    // ExtValue[0].x = 아웃라인 두께 (0 이하면 해당 오브젝트는 아웃라인 없음)
    // ExtValue[0].yzw = 아웃라인 RGB 색상
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];
    float width = data.ExtValue[0].x;
    float3 color = data.ExtValue[0].yzw;
    
    if (width <= 0.0f)
        discard;

    return float4(color, 1.0f);
}
