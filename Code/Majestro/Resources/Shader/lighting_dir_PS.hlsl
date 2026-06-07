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

    float visibility = CalculateCSMShadow(viewPos, viewNormal, light.direction.xyz);
    color.diffuse  *= visibility;
    color.specular *= visibility;


    float3 V_view = normalize(-viewPos);
    IBLResult ibl = CalculateIBLAmbient(viewNormal, V_view, baseColor, metallic, roughness);


    float ao = Gbuffer[4].Sample(g_sam_0, input.uv).a;


    PASS_CUSTOM_DATA passData = PassCustomTable[GlobalParams.PassCustomIndex];
    int aoIdx = passData.ExtTex[0];
    if (aoIdx >= 0)
    {
        ao *= TextureMaps[aoIdx].Sample(g_sam_0, input.uv).r;
    }

    // IBL도 그림자 영역에서 완전히 차단하지 않고 최소값(0.15)을 유지하면서 감쇠
    float iblVisibility = lerp(0.55f, 1.f, visibility);
    output.diffuse  = color.diffuse  + float4(ibl.diffuse  * ao * iblVisibility, 1.0f);
    output.specular = color.specular + float4(ibl.specular * ao * iblVisibility, 0.0f);
    output.diffuse.rgb += light.color.ambient.rgb * ao;
    return output;
}
