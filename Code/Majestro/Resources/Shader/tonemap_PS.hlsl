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
//   ExtValue[0] = (Saturation, Contrast, Brightness, Exposure)
//                  채도(1=기본)  대비(1=기본)  밝기(0=기본)  노출(1=기본, 톤매핑 전 적용)
//   ExtValue[1] = (ShadowTint.rgb,  ShadowStrength)   어두운 영역 색조
//   ExtValue[2] = (MidtoneTint.rgb, MidtoneStrength)  중간 영역 색조
//   ExtValue[3] = (HighlightTint.rgb, HighlightStrength) 밝은 영역 색조
// ──────────────────────────────────────────────────────────────
float3 ApplyColorGrading(float3 color, PASS_CUSTOM_DATA data)
{
    float saturation = data.ExtValue[0].x; // 1.0 = 기본
    float contrast   = data.ExtValue[0].y; // 1.0 = 기본
    float brightness = data.ExtValue[0].z; // 0.0 = 기본

    // 밝기 보정
    color = saturate(color + brightness);

    // 대비 보정 (0.5 기준 선형 스케일)
    color = saturate((color - 0.5f) * contrast + 0.5f);

    // 채도 보정 (Rec.2020 휘도 가중치)
    float lum = dot(color, float3(0.2627f, 0.6780f, 0.0593f));
    color = saturate(lerp(lum.xxx, color, saturation));

    // Shadow / Midtone / Highlight 색조
    float3 shadowTint     = data.ExtValue[1].rgb;
    float  shadowStr      = data.ExtValue[1].w;
    float3 midtoneTint    = data.ExtValue[2].rgb;
    float  midtoneStr     = data.ExtValue[2].w;
    float3 highlightTint  = data.ExtValue[3].rgb;
    float  highlightStr   = data.ExtValue[3].w;

    // 각 영역 마스크 (0~1)
    float curLum = dot(color, float3(0.2627f, 0.6780f, 0.0593f));
    float shadowMask    = 1.0f - smoothstep(0.0f,  0.45f, curLum);
    float highlightMask = smoothstep(0.55f, 1.0f,  curLum);
    float midtoneMask   = saturate(1.0f - shadowMask - highlightMask);

    color += shadowTint    * shadowMask    * shadowStr;
    color += midtoneTint   * midtoneMask   * midtoneStr;
    color += highlightTint * highlightMask * highlightStr;
    color = saturate(color);

    return color;
}

// ──────────────────────────────────────────────────────────────
// 컬러 LUT (언랩 스트립)

// LUT 텍스처가 512x32와 같이 여러 행으로 나뉘어 있을 경우, 각 행을 logicalSize로 나누어 사용
float3 ApplyScaledColorLUT(int lutIdx, float logicalSize, float textureWidth, float textureHeight, float3 color)
{
    color = saturate(color);

    float tileWidth = textureWidth / logicalSize;
    float scaleX = tileWidth / logicalSize;
    float scaleY = textureHeight / logicalSize;

    float bluePos = color.b * (logicalSize - 1.0f);
    float slice0 = floor(bluePos);
    float slice1 = min(slice0 + 1.0f, logicalSize - 1.0f);
    float fBlue = bluePos - slice0;

    float uLocal = (color.r * (logicalSize - 1.0f) + 0.5f) * scaleX;
    float v = (color.g * (logicalSize - 1.0f) + 0.5f) * scaleY / textureHeight;

    float3 c0 = TextureMaps[lutIdx].SampleLevel(g_sam_Terrain, float2((slice0 * tileWidth + uLocal) / textureWidth, v), 0).rgb;
    float3 c1 = TextureMaps[lutIdx].SampleLevel(g_sam_Terrain, float2((slice1 * tileWidth + uLocal) / textureWidth, v), 0).rgb;
    return lerp(c0, c1, fBlue);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[2];

    float3 hdrColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv).rgb;
    bool customColorGradingEnabled = data.ExtTex[5] != 0;

    // 노출 보정 (톤매핑 전 HDR 공간에서 적용)
    float exposure = data.ExtValue[0].w; // 1.0 = 기본
    if (!customColorGradingEnabled)
        exposure = 1.0f;
    hdrColor *= exposure;

    // 톤매핑 (HDR -. LDR)
    float3 mapped = Tonemap_Uchimura(hdrColor);

    // 감마 보정 (선형 -. > sRGB)
    float3 gammaCorrected = LinearToSRGB(saturate(mapped));

    // 컬러 그레이딩 (커스텀)
    if (customColorGradingEnabled)
        gammaCorrected = ApplyColorGrading(gammaCorrected, data);

    // 컬러 LUT
    int lutIdx = data.ExtTex[0];
    if (lutIdx >= 0)
    {
        float N = (float) data.ExtTex[1];
        float strength = data.ExtTex[2] / 100.0f;
        float textureWidth = (float) data.ExtTex[3];
        float textureHeight = (float) data.ExtTex[4];
        if (N > 1.0f && textureWidth > 0.0f && textureHeight > 0.0f)
        {
            float3 graded = ApplyScaledColorLUT(lutIdx, N, textureWidth, textureHeight, gammaCorrected);
            gammaCorrected = lerp(gammaCorrected, graded, strength);
        }
    }
    // 비네트
    //float2 d = input.uv - float2(0.5f, 0.5f);
    //d.x *= PassParams.ScreenSize.x / PassParams.ScreenSize.y; // 화면 종횡비 보정
    //float dist = length(d);

    //float vignette = 1.0f - smoothstep(0.35f, 0.60f, dist);
    //gammaCorrected.rgb *= vignette;
    
    
    
    // gradient overlay
    float mask = smoothstep(0.9f, 1.0f, input.uv.y);
    float3 overlayColor = float3(0.0f, 0.0f, 0.0f);

    gammaCorrected.rgb = lerp(gammaCorrected.rgb, overlayColor, mask * 0.8f);
    
    return float4(gammaCorrected, 1.0f);
}
