#include "params.hlsl"

struct VS_OUT
{
    float4 pos   : SV_Position;
    float2 uv    : TEXCOORD0;
    float3 color : COLOR0;
    float alpha  : TEXCOORD1;
    float ageRate : TEXCOORD2;
    float intensity : TEXCOORD3;
   
    float texIndex : TEXCOORD4; // WeaponTrail 텍스처(-1 = 텍스처 미사용)
};

float WeaponTrailHash(float2 value)
{
    return frac(sin(dot(value, float2(127.1f, 311.7f))) * 43758.5453f);
}

float WeaponTrailNoise(float2 value)
{
    float2 cell = floor(value);
    float2 local = frac(value);
    local = local * local * (3.0f - 2.0f * local);

    float a = WeaponTrailHash(cell);
    float b = WeaponTrailHash(cell + float2(1.0f, 0.0f));
    float c = WeaponTrailHash(cell + float2(0.0f, 1.0f));
    float d = WeaponTrailHash(cell + float2(1.0f, 1.0f));

    return lerp(lerp(a, b, local.x), lerp(c, d, local.x), local.y);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float side = 1.0f - abs(input.uv.x * 2.0f - 1.0f);
    side = saturate(side);

    float time = PassParams.TotalTime;
    float flowNoise = WeaponTrailNoise(float2(input.uv.y * 14.0f - time * 9.0f, input.uv.x * 5.0f));
    float smallNoise = WeaponTrailNoise(float2(input.uv.y * 37.0f + time * 17.0f, input.uv.x * 11.0f));
    float flameNoise = saturate(flowNoise * 0.65f + smallNoise * 0.35f);

    float ageFade = saturate(1.0f - input.ageRate);
    float edgeFade = pow(side, 0.55f);
    float coreMask = pow(side, 2.4f) * (0.65f + flameNoise * 0.35f);
    float headBoost = lerp(0.75f, 1.25f, saturate(input.uv.y));

    float3 emberColor = float3(1.2f, 0.08f, 0.01f);
    float3 fireColor = input.color * 2.0f;
    float3 coreColor = float3(4.5f, 2.7f, 0.9f);

    float3 color = lerp(emberColor, fireColor, edgeFade);
    color = lerp(color, coreColor, saturate(coreMask));
    color *= input.intensity * headBoost;

    float alpha = saturate(input.alpha * ageFade * edgeFade);
    alpha *= lerp(0.65f, 1.15f, flameNoise);

   
    if (input.texIndex >= 0.0f)
    {
        float4 trailTex = TextureMaps[(uint)input.texIndex].Sample(g_sam_0, input.uv);
        color *= trailTex.rgb;
        alpha *= trailTex.a;
    }

    return float4(color * alpha, alpha);
}
