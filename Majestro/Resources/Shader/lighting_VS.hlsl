#include "params.hlsli"
#include "utils.hlsli"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};


// [Directional Light]
// g_int_0 : Light index
// g_tex_0 : Position RT
// g_tex_1 : Normal RT
// g_tex_2 : Shadow RT
// g_mat_0 : ShadowCamera VP
// Mesh : Rectangle

VS_OUT VS_DirLight(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    output.pos = float4(input.pos * 2.f, 1.f);
    output.uv = input.uv;

    return output;
}


// [Point Light]
// g_int_0 : Light index
// g_tex_0 : Position RT
// g_tex_1 : Normal RT
// g_vec2_0 : RenderTarget Resolution   랜더타켓의 해상도
// Mesh : Sphere    포인트 라이트의 영역(구)

VS_OUT VS_PointLight(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    LIGHTINFO light = Lights[GlobalParams.LightIndex];
    
    
    output.pos = mul(float4(input.pos, 1.f), mul(light.MatWorld, mul(PassParams.MatView, PassParams.MatProjection)));
    output.uv = input.uv;

    return output;
}


// [Final]
// g_tex_0 : Diffuse Color Target
// g_tex_1 : Diffuse Light Target
// g_tex_2 : Specular Light Target
// Mesh : Rectangle

VS_OUT VS_Final(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    output.pos = float4(input.pos * 2.f, 1.f);
    output.uv = input.uv;

    return output;
}
