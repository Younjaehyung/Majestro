#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    
    uint instanceID : InstanceID;
};

// [Point Light]
// g_int_0 : Light index
// g_tex_0 : Position RT
// g_tex_1 : Normal RT
// g_vec2_0 : RenderTarget Resolution   랜더타켓의 해상도
// Mesh : Sphere    포인트 라이트의 영역(구)

VS_OUT VS_PointLight(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    output.instanceID = input.instanceID;

    RENDERPARAMS Instance = InstanceParams[input.instanceID];
    
    
    LIGHTINFO light = Lights[Instance.LightIndex];
    
    
    output.pos = mul(float4(input.pos, 1.f), mul(light.MatWorld, mul(PassParams.MatView, PassParams.MatProjection)));
    output.uv = input.uv;

    return output;
}

