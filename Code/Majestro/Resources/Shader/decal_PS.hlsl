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

struct VS_OUT { float4 pos : SV_Position; };

void DecalBasis(float3 n, out float3 F, out float3 R, out float3 U)
{
    F = normalize(n);
    float3 ref = (abs(F.y) > 0.99f) ? float3(0.0f, 0.0f, 1.0f) : float3(0.0f, 1.0f, 0.0f);
    R = normalize(cross(ref, F));
    U = normalize(cross(F, R));
}

float3 ReconstructWorldPos(float2 uv, float deviceDepth)
{
    float3 viewPos = ReconstructViewPos(uv, deviceDepth);
    return mul(float4(viewPos, 1.0f), PassParams.MatViewInv).xyz;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    int3 pixel = int3((int2) input.pos.xy, 0);

    float rtW, rtH;
    Gbuffer[0].GetDimensions(rtW, rtH);
    float2 uv    = input.pos.xy / float2(rtW, rtH);

    float  depth = Gbuffer[0].Load(pixel).r;
    float3 wpos  = ReconstructWorldPos(uv, depth);

    // 표면 노멀(뎁스 유도)
    float3 nWorld = normalize(cross(ddx(wpos), ddy(wpos)));

    if (depth >= 1.0f)
        discard;

    float3 F, R, U;
    DecalBasis(Decal.Normal, F, R, U);

    float3 dvec = wpos - Decal.Center;
    float  lx   = dot(dvec, R) / max(Decal.Radius, 1e-4f);
    float  ly   = dot(dvec, U) / max(Decal.Radius, 1e-4f);
    float  lz   = dot(dvec, F) / max(Decal.HalfDepth, 1e-4f);
    if (abs(lx) > 1.0f || abs(ly) > 1.0f || abs(lz) > 1.0f)
        discard;

    if (abs(dot(nWorld, F)) < Decal.NormalThreshold)
        discard;

    float3 rgb;
    float  alpha;

    int texIdx = (int) Decal.TexIndex;
    if (texIdx >= 0)
    {
        // 데칼 로컬 UV [0,1]
        float2 local = float2(0.5f - lx * 0.5f, 0.5f - ly * 0.5f);

        // 아틀라스 슬라이싱
        int packed = (int) (Decal.Thickness + 0.5f);
        int grid   = max(packed & 63, 1);   // 하위 6비트 = 한 변 셀 수
        int cell   = packed >> 6;           // 나머지 = 셀 인덱스
        float inv  = 1.0f / grid;
        int col    = cell % grid;
        int row    = cell / grid;
        float2 tuv = (float2(col, row) + local) * inv;

        float4 texel = TextureMaps[texIdx].Sample(g_sam_0, tuv);
        rgb   = Decal.Color.rgb * texel.rgb;
        alpha = texel.a * Decal.Color.a * Decal.FadeAlpha;
    }
    else
    {
        float r     = length(float2(lx, ly));
        float halfT = max(Decal.Thickness / max(Decal.Radius, 1e-4f), 1e-4f) * 0.5f;
        float ringR = 1.0f - halfT;
        float aa    = fwidth(r) + 1e-5f;
        float ring  = 1.0f - smoothstep(halfT - aa, halfT + aa, abs(r - ringR));
        rgb   = Decal.Color.rgb;
        alpha = ring * Decal.Color.a * Decal.FadeAlpha;
    }

    if (alpha <= 0.001f)
        discard;

    return float4(rgb, alpha);
}
