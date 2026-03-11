#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
};

void PS_Main(VS_OUT input)
{
    // Depth Pre-Pass: 컬러 출력 없음, 깊이만 기록
}
