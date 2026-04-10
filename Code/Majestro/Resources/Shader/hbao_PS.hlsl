#include "params.hlsl"
#include "math.hlsl"

static const float kTileAngles[16] =
{
    0.0000f, 1.5708f, 0.7854f, 2.3562f,
    2.0944f, 0.5236f, 2.8798f, 1.3090f,
    1.0472f, 2.6180f, 0.2618f, 1.8326f,
    3.1416f, 0.7854f, 2.3562f, 0.0000f,
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float radius    = data.ExtValue[0].x;
    float bias      = data.ExtValue[0].y;
    float intensity = data.ExtValue[0].z;
    int   numSteps  = (int)data.ExtValue[0].w;
    int   numDirs   = (int)data.ExtValue[1].x;
    float texelW    = data.ExtValue[1].y;
    float texelH    = data.ExtValue[1].z;
    float falloff   = data.ExtValue[1].w;

    float3 posVS    = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;
    float3 normalVS = normalize(Gbuffer[2].Sample(g_sam_0, input.uv).xyz);

    if (posVS.z <= 0.0f) return 1.0f;

   
    static const float MAX_PX = 150.0f;
    float screenRadiusW = min((radius / posVS.z) / texelW, MAX_PX);
    float screenRadiusH = min((radius / posVS.z) / texelH, MAX_PX);

    
    uint px = (uint)input.pos.x & 3u;
    uint py = (uint)input.pos.y & 3u;
    float randAngle = kTileAngles[py * 4u + px];

    float ao = 0.0f;

    [loop]
    for (int i = 0; i < numDirs; i++)
    {
        float angle = randAngle + (float)i * (PI / (float)numDirs);
        float cosA  = cos(angle);
        float sinA  = sin(angle);

        float stepW = screenRadiusW / (float)numSteps * texelW;
        float stepH = screenRadiusH / (float)numSteps * texelH;
        float2 dir2D = float2(cosA * stepW, sinA * stepH);

        float maxSinH = 0.0f;

        [loop]
        for (int j = 1; j <= numSteps; j++)
        {

            float2 sUV = input.uv + dir2D * (float)j;
            if (any(sUV < 0.0f) || any(sUV > 1.0f)) continue;

            float3 sPosVS = Gbuffer[1].Sample(g_sam_0, sUV).xyz;
            if (sPosVS.z <= 0.0f) continue;

            float3 diff = sPosVS - posVS;
            float  dist = length(diff);
            if (dist < 0.001f || dist > radius) continue;

            float sinH_depth = (posVS.z - sPosVS.z) / dist;

            
            float hemiMask = saturate(dot(normalVS, normalize(diff)) * 0.5f + 0.5f);

            float sinH        = sinH_depth * hemiMask;
            float attenuation = saturate(1.0f - pow(dist / radius, falloff));
            maxSinH = max(maxSinH, sinH * attenuation);
        }

        ao += saturate(maxSinH - bias);
    }

    ao = (ao / (float)numDirs) * intensity;
    return saturate(1.0f - ao);
}
