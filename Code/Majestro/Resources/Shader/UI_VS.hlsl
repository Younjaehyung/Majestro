
#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION; // (x, y, z) = pixel 좌표
    float2 uv : TEXCOORD;
    
};

struct VS_OUT
{
    float4 pos : SV_POSITION; // 클립 공간
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

    uint idx = GlobalParams.etc;
    RENDERPARAMS instance = InstanceParams[idx];
    int object = instance.ObjectIndex;
    
    
    UIInstanceData inst = UIInstances[object];
    
    // 1. Pivot 보정
    float2 local = input.pos.xy - inst.pivot;

    // 2. 로컬 -> 픽셀
    float2 pixelPos = inst.position + local * inst.size;

    // 3. 픽셀 -> NDC
    float2 ndc;
    ndc.x = (pixelPos.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (pixelPos.y / PassParams.ScreenSize.y) * 2.0f;

    output.pos = float4(ndc, inst.zOrder, 1.0f);
    output.uv = input.uv;

    return output;
}
