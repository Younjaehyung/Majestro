#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float Hash21(float2 p)
{
    p = frac(p * float2(127.1f, 311.7f));
    p += dot(p, p + 34.23f);
    return frac(p.x * p.y);
}

float2 Hash22(float2 p)
{
    return float2(Hash21(p + 17.13f), Hash21(p + 53.71f));
}



float IrregularTriangleMask(float2 p, float2 a, float2 b, float2 c)
{
    float areaSign = sign(Cross2(b - a, c - a));
    areaSign = areaSign == 0.0f ? 1.0f : areaSign;

    float e0 = Cross2(b - a, p - a) * areaSign;
    float e1 = Cross2(c - b, p - b) * areaSign;
    float e2 = Cross2(a - c, p - c) * areaSign;
    float edge = min(min(e0, e1), e2);

    // 최초 셰이더의 삼각형 가시성을 유지하는 경계 부드러움 값
    return smoothstep(0.0f, 0.0045f, edge);
}

float3 MakeTriangleColor(float2 uv, float4 themeData)
{
    float aspect = max(PassParams.ScreenSize.x / max(PassParams.ScreenSize.y, 1.0f), 1.0f);
    float2 centered = uv - float2(0.5f, 0.5f);
    centered.x *= aspect;

    float topWeight = smoothstep(0.0f, 1.0f, uv.y);
    float3 topColor = float3(0.82f, 0.86f, 0.92f);
    float3 bottomColor = float3(0.20f, 0.21f, 0.23f);
    float3 baseColor = lerp(bottomColor, topColor, topWeight);

    float centerLight = 1.0f - smoothstep(0.04f, 0.62f, length(centered));
    baseColor = lerp(baseColor, float3(0.72f, 0.74f, 0.76f), centerLight * 0.45f);

    float time = PassParams.TotalTime;
    float3 color = baseColor;

    // 최초 셰이더와 동일한 개수와 배치로 삼각형 형태를 복원한다
    [unroll]
    for (int i = 0; i < 32; ++i)
    {
        float fi = (float)i;
        float2 seed = float2(fi * 3.17f + 0.23f, fi * 7.91f + 4.61f);
        float2 rnd0 = Hash22(seed);
        float2 rnd1 = Hash22(seed + 11.0f);
        float2 rnd2 = Hash22(seed + 29.0f);
        float2 rnd3 = Hash22(seed + 48.0f);

        float2 center = float2(lerp(-0.16f, 1.16f, rnd0.x), lerp(-0.10f, 1.18f, rnd0.y));
        float radius = lerp(0.24f, 0.56f, rnd1.x);
        float stretch = lerp(0.55f, 1.85f, rnd1.y);
        float angle = rnd2.x * 6.28318f;

        float2 dir0 = float2(cos(angle), sin(angle));
        float2 dir1 = float2(cos(angle + lerp(1.65f, 2.55f, rnd2.y)), sin(angle + lerp(1.65f, 2.55f, rnd2.y)));
        float2 dir2 = float2(cos(angle - lerp(1.35f, 2.35f, rnd3.x)), sin(angle - lerp(1.35f, 2.35f, rnd3.x)));

        dir0.x /= aspect;
        dir1.x /= aspect;
        dir2.x /= aspect;

        float2 a = center + dir0 * radius * stretch;
        float2 b = center + dir1 * radius * lerp(0.65f, 1.35f, rnd3.y);
        float2 c = center + dir2 * radius * lerp(0.75f, 1.55f, rnd0.y);

        float mask = IrregularTriangleMask(uv, a, b, c);
        float shade = rnd1.y * 2.0f - 1.0f;
        
        // 삼각형 형태는 고정하고 화면을 가로지르는 밝기 파동만 시간에 따라 이동시킨다
        float travelingWave = sin(uv.x * 4.0f + uv.y * 2.0f - time * 0.6f + rnd3.x * 0.65f);
        float localPulse = sin(time * lerp(0.75f, 1.05f, rnd2.y) + rnd3.x * 6.28318f) * 0.5f + 0.5f;
        float shimmer = saturate(travelingWave * 0.72f + localPulse * 0.28f);

        // 최초 셰이더의 밝고 어두운 면 대비를 그대로 복원한다
        float3 lightTri = float3(0.94f, 0.96f, 1.0f);
        float3 darkTri = float3(0.22f, 0.22f, 0.30f);
        float3 triColor = shade > 0.0f
            ? lerp(baseColor, lightTri, shade * 0.34f)
            : lerp(baseColor, darkTri, -shade * 0.45f);

        // 선택 캐릭터 색상은 원래 명암을 지우지 않을 정도로만 삼각형 면에 적용한다
        float faceLuminance = dot(triColor, float3(0.2126f, 0.7152f, 0.0722f));
        float3 themedFace = themeData.rgb * max(faceLuminance, 0.18f);
        triColor = lerp(triColor, themedFace, saturate(themeData.a * (0.10f + shimmer * 0.18f)));
        triColor = lerp(triColor, lightTri, shimmer * 0.24f);

        // 최초 혼합 강도를 복원하고 파동이 지나갈 때 해당 삼각형만 더 선명해지게 한다
        float layerAlpha = lerp(0.035f, 0.12f, rnd0.x) + shimmer * 0.065f;
        color = lerp(color, triColor, mask * layerAlpha * 5.0f);
    }

    float vignette = 1.0f - smoothstep(0.46f, 0.95f, length(centered));
    color *= lerp(0.72f, 1.0f, vignette);

    float floorBand = smoothstep(0.0f, 0.24f, uv.y);
    color = lerp(color * 0.55f, color, floorBand);

    return color;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    float4 themeData = PassCustomTable[GlobalParams.PassCustomIndex].ExtValue[0];
    float3 backgroundColor = MakeTriangleColor(input.uv, themeData);

    // 전체 배경에는 약한 색상만 적용하고 강한 캐릭터 색상은 삼각형 면에 집중한다
    float luminance = dot(backgroundColor, float3(0.2126f, 0.7152f, 0.0722f));
    float3 themedColor = luminance * themeData.rgb;
    backgroundColor = lerp(backgroundColor, themedColor, saturate(themeData.a * 0.28f));

    return float4(backgroundColor, 1.0f);
}
