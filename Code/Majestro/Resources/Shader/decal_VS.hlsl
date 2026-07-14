#define MJ_OVERRIDE_GLOBAL_PARAMS
#include "params.hlsl"

struct DECAL_PARAMS
{
    float3 Center;   
    float Radius;
    float3 Normal;   
    float HalfDepth;
    float4 Color;
    float  FadeAlpha; float NormalThreshold; float Thickness; float TexIndex;
};

ConstantBuffer<DECAL_PARAMS> Decal : register(b0, space0);

void DecalBasis(float3 n, out float3 F, out float3 R, out float3 U)
{
    F = normalize(n);
    float3 ref = (abs(F.y) > 0.99f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    R = normalize(cross(ref, F));
    U = normalize(cross(F, R));
}

struct VS_IN  { float3 pos : POSITION; };
struct VS_OUT { float4 pos : SV_Position; };

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    float3 F, R, U;
    DecalBasis(Decal.Normal, F, R, U);

    float3 lp = input.pos * float3(Decal.Radius * 2.0f, Decal.Radius * 2.0f, Decal.HalfDepth * 2.0f);
    float3 worldPos = Decal.Center + lp.x * R + lp.y * U + lp.z * F;

    float4 viewPos = mul(float4(worldPos, 1.0f), PassParams.MatView);
    output.pos     = mul(viewPos, PassParams.MatProjection);
    return output;
}
