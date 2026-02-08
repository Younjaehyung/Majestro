
#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};
float4 PS_Main(VS_OUT input) : SV_TARGET
{
    const int cubeMapIndex = PassParams.SkyBoxIndex;

    float3 sampleDir = normalize(input.dir);
    float4 uvw = float4(sampleDir, (float) cubeMapIndex);
    
    float4 color = CubeBoxMaps.SampleLevel(g_sam_0, uvw, 0.0f);

    // HDRI 톤매핑(간단한 Reinhard)
    color.rgb = color.rgb / (1.0f + color.rgb);


    return color;
}