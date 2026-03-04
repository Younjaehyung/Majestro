#include "params.hlsl"
#include "utils.hlsl"

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

// [Final]
// g_tex_0 : Diffuse Color Target
// g_tex_1 : Diffuse Light Target
// g_tex_2 : Specular Light Target
// Mesh : Rectangle




float4 PS_Final(VS_OUT input) : SV_Target
{
    float4 output = (float4) 0;

    float4 lightPower = Gbuffer[4].Sample(g_sam_0, input.uv);
    if (lightPower.x == 0.f && lightPower.y == 0.f && lightPower.z == 0.f)
        clip(-1);

    float4 color = Gbuffer[3].Sample(g_sam_0, input.uv);
    float4 specular = Gbuffer[5].Sample(g_sam_0, input.uv);


    
    output = (color * lightPower) * specular.a;
    return output;
}
