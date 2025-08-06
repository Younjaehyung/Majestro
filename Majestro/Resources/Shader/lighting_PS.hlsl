#include "params.hlsli"
#include "utils.hlsli"

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

// [Point Light]
// g_int_0 : Light index
// g_tex_0 : Position RT
// g_tex_1 : Normal RT
// g_vec2_0 : RenderTarget Resolution   랜더타켓의 해상도
// Mesh : Sphere    포인트 라이트의 영역(구)

PS_OUT PS_PointLight(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    // input.pos = SV_Position = Screen 좌표 (정규화 되고 변환되었기 때문에 좌표계가 픽셀좌표계로 변환되어있음)
    float2 uv = float2(input.pos.x / PassParams.ScreenSize.x, input.pos.y / PassParams.ScreenSize.y);
    float3 viewPos = Gbuffer[1].Sample(g_sam_0, uv).xyz;
    if (viewPos.z <= 0.f)
        clip(-1);

    int lightIndex = GlobalParams.LightIndex;
    float3 viewLightPos = mul(float4(Lights[lightIndex].position.xyz, 1.f), PassParams.MatView).xyz; //position: 빛의 원래 좌표   
    float distance = length(viewPos - viewLightPos);
    if (distance > Lights[lightIndex].range)   // 원래좌표와 빛의 좌표를 이용해서 구 내부인지 확인
        clip(-1);

    float3 viewNormal = Gbuffer[2].Sample(g_sam_0, uv).xyz;

    LightColor color = CalculateLightColor(lightIndex, viewNormal, viewPos);

    output.diffuse = color.diffuse + color.ambient;
    output.specular = color.specular;

    return output;
}

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

    output = (color * lightPower) + specular;
    return output;
}
