#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

static const float SIGMA_Z   = 0.3f; 
static const int   MAX_RADIUS = 5;   

float GaussianWeight(float x, float sigma)
{
    return exp(-0.5f * (x * x) / (sigma * sigma));
}

float PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];
    uint             dir  = GlobalParams.etc; 

    float  texelW     = data.ExtValue[dir].x;
    float  texelH     = data.ExtValue[dir].y;
    uint   srcIdx     = (uint)data.ExtValue[dir].z;
    float  blurRadius = data.ExtValue[dir].w;


    float2 blurDir = (dir == 0)
        ? float2(texelW, 0.0f)
        : float2(0.0f, texelH);


    float centerAo    = TextureMaps[srcIdx].Sample(g_sam_0, input.uv).r;
    float centerDepth = Gbuffer[1].Sample(g_sam_0, input.uv).z; 


    if (centerDepth <= 0.0f)
        return 1.0f;

    float weightedSum = centerAo * 1.0f;
    float totalWeight = 1.0f;

    int tapCount = clamp((int)blurRadius, 1, MAX_RADIUS);

    [loop]
    for (int i = -tapCount; i <= tapCount; i++)
    {
        if (i == 0) continue;

        float2 sUV    = saturate(input.uv + blurDir * (float)i);
        float  sAo    = TextureMaps[srcIdx].Sample(g_sam_0, sUV).r;
        float  sDepth = Gbuffer[1].Sample(g_sam_0, sUV).z;

    
        if (sDepth <= 0.0f) continue;


        float wSpatial = GaussianWeight((float)abs(i), blurRadius * 0.5f);


        float depthDiff = abs(sDepth - centerDepth);

        float wDepth = GaussianWeight(depthDiff / (centerDepth * 0.01f + 0.1f), SIGMA_Z);

        float w = wSpatial * wDepth;
        weightedSum += sAo * w;
        totalWeight += w;
    }

    return weightedSum / max(totalWeight, 1e-5f);
}
