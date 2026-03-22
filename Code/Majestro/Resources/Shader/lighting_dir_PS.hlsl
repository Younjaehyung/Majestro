#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    uint instanceID : InstanceID;
};

struct PS_OUT
{
    float4 diffuse : SV_Target0;
    float4 specular : SV_Target1;
};


// Gbuffer[0] : Shadow depth
// Gbuffer[1] : Position (view-space xyz)
// Gbuffer[2] : Normal+Metallic (xyz = view normal, w = metallic)  
// Gbuffer[3] : Albedo+Roughness (rgb = baseColor, a = roughness)  

PS_OUT PS_DirLight(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;


    int index = -1;
    [loop]
    for (int i = 0; i < PassParams.LightsCount; ++i)
    {
        if (Lights[i].lightType == 0)
        {
            index = i;
            break;
        }
    }


    if (index < 0)
    {
        return output;
    }

    LIGHTINFO light = Lights[index];


    float3 viewPos = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;

    if (viewPos.z <= 0.f)
        clip(-1);

    float4 n_m = Gbuffer[2].Sample(g_sam_0, input.uv);
    float3 viewNormal = normalize(n_m.xyz);
    float metallic = saturate(n_m.w);

    float4 a_r = Gbuffer[3].Sample(g_sam_0, input.uv); 
    float3 baseColor = a_r.rgb;
    float roughness = saturate(a_r.a);


    LightColor color = CalculateLightColorPBR(index, viewNormal, viewPos, baseColor, metallic, roughness);


    if (length(color.diffuse.rgb) != 0)
    {
        float visibility = CalculateCSMShadow(viewPos, viewNormal, light.direction.xyz);
        color.diffuse *= visibility;
        color.specular *= visibility;
    }


    output.diffuse = color.diffuse + color.ambient;
    output.specular = color.specular;

    return output;
}
