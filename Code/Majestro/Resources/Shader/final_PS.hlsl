#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

// [Final]
// g_tex_0 : Diffuse Color Target
// g_tex_1 : Diffuse Light Target
// g_tex_2 : Specular Light Target
// Mesh : Rectangle




float4 PS_Final(VS_OUT input) : SV_Target
{
    float4 output = (float4) 0;

    float4 lightPower = Gbuffer[4].Sample(g_sam_0, input.uv);
    float4 colorRaw   = Gbuffer[3].Sample(g_sam_0, input.uv);
    float4 specular   = Gbuffer[5].Sample(g_sam_0, input.uv);

    // ── 에미시브 픽셀 판별 ────────────────────────────────────────────────
    // deferred_PS에서 에미시브가 있으면 Gbuffer[3].a < 0 으로 플래깅됨.
    // 에미시브 픽셀: 조명 곱셈 없이 에미시브 색을 그대로 HDR에 기여.
    // 일반 픽셀: albedo * diffuseLight + specular (기존 deferred shading).
    // ─────────────────────────────────────────────────────────────────────
    if (colorRaw.a < 0.f)
    {
        // 에미시브 픽셀 — 조명 영향 없이 순수 HDR 발광 값 출력
        // 추가로 specular(IBL)도 더해 환경광 반사는 유지
        output = float4(colorRaw.rgb + specular.rgb, 1.f);
    }
    else
    {
        // 일반 픽셀
        if (lightPower.x == 0.f && lightPower.y == 0.f && lightPower.z == 0.f)
            clip(-1);
        output = (colorRaw * lightPower) + specular;
    }

    return output;
}
