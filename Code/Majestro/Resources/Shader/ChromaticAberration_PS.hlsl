#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


float4 PS_Main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;
    
    // 각 채널별 샘플링 UV 계산
    // R채널은 바깥쪽으로, B채널은 안쪽으로 당겨서 분리감 표현
    float2 offset = CalcRadialOffset(uv, 0.03f);
    
    float2 uvR = uv + offset; // R: 바깥쪽
    float2 uvG = uv; // G: 원본 (기준점)
    float2 uvB = uv - offset; // B: 안쪽
    
    // UV 클램핑 (화면 밖 샘플링 방지)
    uvR = saturate(uvR);
    uvG = saturate(uvG);
    uvB = saturate(uvB);
    
    float r = Gbuffer[9].Sample(g_sam_0, uvR).r;
    float g = Gbuffer[9].Sample(g_sam_0, uvG).g;
    float b = Gbuffer[9].Sample(g_sam_0, uvB).b;
    float a = Gbuffer[9].Sample(g_sam_0, uvG).a;
    
    return float4(r, g, b, a);
}
