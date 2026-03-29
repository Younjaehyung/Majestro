#include "params.hlsl"
#include "utils.hlsl"
struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float3 ReinhardToneMap(float3 hdr)
{
    return hdr / (1.0f + hdr);
}

float3 LinearToSRGB(float3 linearColor)
{
    float3 cutoff = step(linearColor, float3(0.0031308f, 0.0031308f, 0.0031308f));
    float3 lower = linearColor * 12.92f;
    float3 upper = 1.055f * pow(max(linearColor, 1e-5f), 1.0f / 2.4f) - 0.055f;

    return lerp(upper, lower, cutoff);
}

float3 Tonemap_Uchimura(float3 x)
{
    static const float P = 1.0f;
    static const float a = 1.0f;
    static const float m = 0.22f;
    static const float l = 0.4f;
    static const float c = 1.33f;
    static const float b = 0.0f;

    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0f - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float3 w0 = 1.0f - smoothstep(0.0f, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0f - w0 - w2;

    float3 T = m * pow(max(x / m, 0.0f), c) + b; // Toe
    float3 S = P - (P - S1) * exp(CP * (x - S0)); // Shoulder
    float3 L = m + a * (x - m); // Linear

    return T * w0 + L * w1 + S * w2;
}

// ──────────────────────────────────────────────────────────────
// 컬러 그레이딩
//
// PassCustomTable[2].ExtValue 레이아웃:
//   ExtValue[0] = (Saturation, Contrast, Brightness, Enabled)
//                  채도(1=기본)  대비(1=기본)  밝기(0=기본)  (0=비활성)
//   ExtValue[1] = (ShadowTint.rgb,  ShadowStrength)   어두운 영역 색조
//   ExtValue[2] = (MidtoneTint.rgb, MidtoneStrength)  중간 영역 색조
//   ExtValue[3] = (HighlightTint.rgb, HighlightStrength) 밝은 영역 색조
// ──────────────────────────────────────────────────────────────
float3 ApplyColorGrading(float3 color, PASS_CUSTOM_DATA data)
{
    float enabled = data.ExtValue[0].w;
    if (enabled < 0.5f)
        return color;

    float saturation = data.ExtValue[0].x; // 1.0 = 기본
    float contrast   = data.ExtValue[0].y; // 1.0 = 기본
    float brightness = data.ExtValue[0].z; // 0.0 = 기본

    // 밝기 보정
    color = saturate(color + brightness);

    // 대비 보정 (0.5 기준 선형 스케일)
    color = saturate((color - 0.5f) * contrast + 0.5f);

    // 채도 보정 (Rec.709 휘도 가중치)
    float lum = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    color = saturate(lerp(lum.xxx, color, saturation));

    // Shadow / Midtone / Highlight 색조
    float3 shadowTint     = data.ExtValue[1].rgb;
    float  shadowStr      = data.ExtValue[1].w;
    float3 midtoneTint    = data.ExtValue[2].rgb;
    float  midtoneStr     = data.ExtValue[2].w;
    float3 highlightTint  = data.ExtValue[3].rgb;
    float  highlightStr   = data.ExtValue[3].w;

    // 각 영역 마스크 (0~1)
    float curLum        = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float shadowMask    = 1.0f - smoothstep(0.0f,  0.45f, curLum);
    float highlightMask = smoothstep(0.55f, 1.0f,  curLum);
    float midtoneMask   = saturate(1.0f - shadowMask - highlightMask);

    color += shadowTint    * shadowMask    * shadowStr;
    color += midtoneTint   * midtoneMask   * midtoneStr;
    color += highlightTint * highlightMask * highlightStr;
    color = saturate(color);

    return color;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[2];

    float3 hdrColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv).rgb;

    // 톤매핑 (HDR → LDR)
    float3 mapped = Tonemap_Uchimura(hdrColor);

    // 컬러 그레이딩 (톤매핑 후, 감마 보정 전)
    mapped = ApplyColorGrading(mapped, data);

    // 감마 보정
    float3 gammaCorrected = pow(max(mapped, 0.0f), 1.0f / 2.0f);
   // float3 gammaCorrected = pow(max(mapped, 0.0f), 1.0f / 1.8f);
    return float4(gammaCorrected, 1.0f);
}