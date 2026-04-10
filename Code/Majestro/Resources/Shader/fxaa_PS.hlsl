#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

static const int   FXAA_SEARCH_STEPS = 12;


float Luma(float3 rgb)
{
    return dot(rgb, float3(0.2627f, 0.6780f, 0.0593f));
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint  srcIdx           = (uint) data.PreviousStep;
    float edgeThreshold    = data.ExtValue[0].x;
    float edgeThresholdMin = data.ExtValue[0].y;
    float subpixQuality    = data.ExtValue[0].z;

    float2 texelSize = float2(1.0f / PassParams.ScreenSize.x,
                              1.0f / PassParams.ScreenSize.y);
    float2 uv = input.uv;

  
    float3 colorM = Gbuffer[srcIdx].Sample(g_sam_0, uv).rgb;
    float  lumaM  = Luma(colorM);

    float lumaN  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2( 0.0f,         -texelSize.y))).rgb);
    float lumaS  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2( 0.0f,         +texelSize.y))).rgb);
    float lumaE  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x,   0.0f       ))).rgb);
    float lumaW  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x,   0.0f       ))).rgb);

    float lumaMax   = max(max(lumaN, lumaS), max(lumaE, max(lumaW, lumaM)));
    float lumaMin   = min(min(lumaN, lumaS), min(lumaE, min(lumaW, lumaM)));
    float lumaRange = lumaMax - lumaMin;

   
    if (lumaRange < max(edgeThresholdMin, lumaMax * edgeThreshold))
        return float4(colorM, 1.0f);

   
    float lumaNW = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x, -texelSize.y))).rgb);
    float lumaNE = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x, -texelSize.y))).rgb);
    float lumaSW = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(-texelSize.x, +texelSize.y))).rgb);
    float lumaSE = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv + float2(+texelSize.x, +texelSize.y))).rgb);

   
    float edgeH = abs(lumaNW + lumaN + lumaNE - lumaSW - lumaS - lumaSE) * 2.0f
                + abs(lumaNW - lumaSW) + abs(lumaNE - lumaSE);
    float edgeV = abs(lumaNW + lumaW + lumaSW - lumaNE - lumaE - lumaSE) * 2.0f
                + abs(lumaNW - lumaNE) + abs(lumaSW - lumaSE);

    bool isHorizontal = (edgeH >= edgeV);

   
    float luma1 = isHorizontal ? lumaN : lumaW;
    float luma2 = isHorizontal ? lumaS : lumaE;

    float gradient1 = abs(luma1 - lumaM);
    float gradient2 = abs(luma2 - lumaM);

    bool  is1Steeper     = (gradient1 >= gradient2);
    float gradientScaled = 0.25f * max(gradient1, gradient2);

    float stepSize     = isHorizontal ? texelSize.y : texelSize.x;
    float lumaLocalAvg;

    if (is1Steeper)
    {
        stepSize    = -stepSize;
        lumaLocalAvg = 0.5f * (luma1 + lumaM);
    }
    else
    {
        lumaLocalAvg = 0.5f * (luma2 + lumaM);
    }

    
    float2 currentUV = uv;
    if (isHorizontal) currentUV.y += stepSize * 0.5f;
    else              currentUV.x += stepSize * 0.5f;

  
    float2 offset = isHorizontal ? float2(texelSize.x, 0.0f)
                                 : float2(0.0f, texelSize.y);

    float2 uv1 = currentUV - offset;
    float2 uv2 = currentUV + offset;

    float lumaEnd1 = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv1)).rgb) - lumaLocalAvg;
    float lumaEnd2 = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv2)).rgb) - lumaLocalAvg;

    bool reached1 = (abs(lumaEnd1) >= gradientScaled);
    bool reached2 = (abs(lumaEnd2) >= gradientScaled);

    
    [loop]
    for (int i = 0; i < FXAA_SEARCH_STEPS && !(reached1 && reached2); ++i)
    {
        if (!reached1)
        {
            uv1      -= offset;
            lumaEnd1  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv1)).rgb) - lumaLocalAvg;
            reached1  = (abs(lumaEnd1) >= gradientScaled);
        }
        if (!reached2)
        {
            uv2      += offset;
            lumaEnd2  = Luma(Gbuffer[srcIdx].Sample(g_sam_0, saturate(uv2)).rgb) - lumaLocalAvg;
            reached2  = (abs(lumaEnd2) >= gradientScaled);
        }
    }

    
    float dist1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
    float dist2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);

    float edgeLen     = dist1 + dist2;
    float pixelOffset = -min(dist1, dist2) / edgeLen + 0.5f;

    bool lumaMLessAvg  = (lumaM < lumaLocalAvg);
    bool correctVar1   = ((lumaEnd1 < 0.0f) != lumaMLessAvg);
    bool correctVar2   = ((lumaEnd2 < 0.0f) != lumaMLessAvg);
    bool correctVar    = (dist1 < dist2) ? correctVar1 : correctVar2;

    float pixelOffsetFinal = correctVar ? pixelOffset : 0.0f;

  
    float lumaAvg3x3 = (1.0f / 12.0f) * (
        2.0f * (lumaN + lumaS + lumaE + lumaW) +
        lumaNW + lumaNE + lumaSW + lumaSE);

    float subpixOffset1 = saturate(abs(lumaAvg3x3 - lumaM) / lumaRange);
    float subpixOffset2 = subpixOffset1 * subpixOffset1 * subpixQuality;


    float finalOffset = max(pixelOffsetFinal, subpixOffset2);


    float2 finalUV = uv;
    if (isHorizontal) finalUV.y += finalOffset * stepSize;
    else              finalUV.x += finalOffset * stepSize;

    finalUV = saturate(finalUV);

    return float4(Gbuffer[srcIdx].Sample(g_sam_0, finalUV).rgb, 1.0f);
}
