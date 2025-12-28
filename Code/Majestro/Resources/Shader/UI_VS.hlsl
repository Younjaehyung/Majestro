
#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION; // (x, y, z) = pixel 좌표
    float2 uv : TEXCOORD;
    
    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_POSITION; // 클립 공간
    float2 uv : TEXCOORD;

    uint instanceID : InstanceID;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;
	output.instanceID = input.instanceID;
    
    UIInstanceData inst = UIInstances[input.instanceID];
    
    // 1. Pivot 보정
    float2 local = input.pos.xy - inst.Pivot;

    // 2. 로컬 -> 픽셀
    float2 pixelPos = inst.Position + local * inst.Size;

    // 3. 픽셀 -> NDC
    float2 ndc;
    ndc.x = (pixelPos.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (pixelPos.y / PassParams.ScreenSize.y) * 2.0f;

    output.pos = float4(ndc, inst.ZOrder, 1.0f);
    output.uv = input.uv;

    return output;
}
