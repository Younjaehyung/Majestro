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
        color.diffuse  *= visibility;
        color.specular *= visibility;
    }

    // IBL ambient — 기존 단색 ambient를 환경맵 기반으로 대체
    // diffuse  → DiffuseLightTarget  : final pass에서 albedo와 곱해짐
    // specular → SpecularLightTarget : final pass에서 albedo 없이 더해짐 (metallic 왜곡 방지)
    float3 V_view = normalize(-viewPos);
    IBLResult ibl = CalculateIBLAmbient(viewNormal, V_view, baseColor, metallic, roughness);

    // HBAO+ AO 적용: IBL ambient 전용 (직접광에는 영향 없음)
    // LIGHTS_PASS 슬롯(ExtTex[0])에서 AO 텍스처 인덱스 조회
    PASS_CUSTOM_DATA passData = PassCustomTable[GlobalParams.PassCustomIndex];
    float ao = 1.0f;
    int   aoIdx = passData.ExtTex[0];
    if (aoIdx >= 0)
    {
        float2 screenUV = input.uv;
        ao = TextureMaps[aoIdx].Sample(g_sam_0, screenUV).r;
    }

    output.diffuse  = color.diffuse  + float4(ibl.diffuse  * ao, 1.0f);
    output.specular = color.specular + float4(ibl.specular * ao, 0.0f);

    return output;
}
