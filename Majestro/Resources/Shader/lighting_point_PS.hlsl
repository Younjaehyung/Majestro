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
