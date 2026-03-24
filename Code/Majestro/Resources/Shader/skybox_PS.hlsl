
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

    float4 color = CubeBoxMaps[cubeMapIndex].SampleLevel(g_sam_0, sampleDir, 0.0f);


    return color;
}