#include "params.hlsl"

struct VS_OUT
{
    float4 pos                      : SV_Position;
    float2 uv : TEXCOORD0;
};
static const float3 BT709 = float3(0.2126f, 0.7152f, 0.0722f);

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];
    Texture2D distortedTex = Gbuffer[data.ExtTex[0]];
    Texture2D gradientTex = TextureMaps[data.ExtTex[1]];
    
    float4 color = distortedTex.Sample(g_sam_0, input.uv);
    float luminance = dot(color.rgb, BT709); // 0.0 ~ 1.0 범위

    // luminance를 그라디언트 텍스처의 U축 좌표로 사용
    float4 mapped = gradientTex.Sample(g_sam_0, float2(luminance, 0.5f));

    return mapped;
}
