#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

struct PS_OUT
{
    float4 diffuse : SV_Target0;
    float4 specular : SV_Target1;
};

// [Directional Light]
// g_int_0 : Light index
// g_tex_0 : Position RT
// g_tex_1 : Normal RT
// g_tex_2 : Shadow RT
// g_mat_0 : ShadowCamera VP
// Mesh : Rectangle

PS_OUT PS_DirLight(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;
    int index = GlobalParams.LightIndex;
    LIGHTINFO light = Lights[index];
    MATERIALINFO material = Materials[light.MaterialsIndex];
    
    float3 viewPos = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;
    if (viewPos.z <= 0.f)   //DirLight의 영역에 카메라에 있는지에 따라 출력할지 아닐지 정함
        clip(-1); //clip에 값이 0보다 작으면 종료하게 됨

    float3 viewNormal = Gbuffer[2].Sample(g_sam_0, input.uv).xyz;

    LightColor color = CalculateLightColor(index, viewNormal, viewPos);
    
    
    
    // 그림자
    if (length(color.diffuse) != 0)
    {
        
        
        matrix shadowCameraVP = mul(light.MatView, light.MatProjection);

        float4 worldPos = mul(float4(viewPos.xyz, 1.f), light.MatViewInv);
        float4 shadowClipPos = mul(worldPos, shadowCameraVP);
        float depth = shadowClipPos.z / shadowClipPos.w;

        // x [-1 ~ 1] -> u [0 ~ 1]
        // y [1 ~ -1] -> v [0 ~ 1]
        float2 uv = shadowClipPos.xy / shadowClipPos.w;
        uv.y = -uv.y;
        uv = uv * 0.5 + 0.5;

        if (0 < uv.x && uv.x < 1 && 0 < uv.y && uv.y < 1)
        {
            float shadowDepth = Gbuffer[0].Sample(g_sam_0, uv).x;
            if (shadowDepth > 0 && depth > shadowDepth + 0.00001f)
            {
                color.diffuse *= 0.5f;
                color.specular = (float4) 0.f;
            }
        }
    }

    
    output.diffuse = color.diffuse + color.ambient;
    output.specular = color.specular;

    return output;
}
