#include "params.hlsl"

struct VS_OUT
{
    float4 pos                      : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];
    float2 uv = input.uv;

    float4 hdrColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, uv);

    float deviceDepth = Gbuffer[0].Sample(g_sam_0, uv).r;
    float3 fogColor = data.ExtValue[0].xyz;
    
       // 스카이박스/배경
    if (deviceDepth >= 1.0f)
        return float4(lerp(hdrColor.rgb, fogColor, 0.2),hdrColor.a);
    

    float2 ndcXY   = uv * float2(2.f, -2.f) + float2(-1.f, 1.f);
    float4 ndcPos  = float4(ndcXY, deviceDepth, 1.f);
    float4 viewPos = mul(ndcPos, PassParams.MatProjectionInv);
    viewPos /= viewPos.w;
    float viewDepth = viewPos.z; 

      // Distance Fog
    float fogStart = data.ExtValue[1].x;
    float fogEnd   = data.ExtValue[1].y;
    float distFog  = saturate((viewDepth - fogStart) / max(fogEnd - fogStart, 0.001f));

      // Height Fog
    float3 worldPos = mul(viewPos, PassParams.MatViewInv).xyz;
    float fogHeightStart = data.ExtValue[1].z;
    float fogHeightEnd = data.ExtValue[1].w;
    float heightFogWeight = data.ExtValue[2].x;
    float heightFog = saturate(
          (fogHeightStart - worldPos.y) / max(fogHeightStart - fogHeightEnd, 0.001f)
      );


    float fogFactor = saturate(distFog + heightFog * heightFogWeight);

    float3 result = lerp(hdrColor.rgb, fogColor, fogFactor);
    
    return float4(result, hdrColor.a);
}