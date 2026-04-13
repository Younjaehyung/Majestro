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

    // position RT로 배경/지오메트리 구분 (metallic=1이면 diffuse light가 0이 되어 lightPower로 판단 불가)
    float3 posCheck = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;
    if (posCheck.x == 0.f && posCheck.y == 0.f && posCheck.z == 0.f)
        clip(-1);

    float4 lightPower = Gbuffer[5].Sample(g_sam_0, input.uv); // GBUFFER_DIFFUSE_INDEX = 5

    float4 color    = Gbuffer[3].Sample(g_sam_0, input.uv); // GBUFFER_ALBEDO_INDEX = 3
    float4 specular = Gbuffer[6].Sample(g_sam_0, input.uv); // GBUFFER_SPECULAR_INDEX = 6

    
    if (any(Gbuffer[4].Sample(g_sam_0, input.uv).rgb)) // GBUFFER_EMISSIVE_INDEX = 4
    {
        output = Gbuffer[4].Sample(g_sam_0, input.uv) * 10;
        return output;
    }
        output = (color * lightPower) + specular;
    
    
    return output;
   
}
