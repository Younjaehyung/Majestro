
#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION; // 단위 큐브의 로컬 좌표(-1~1)
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 dir : TEXCOORD0;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output;

   
    float4 viewPos = mul(float4(input.pos, 0.0f), PassParams.MatView);
    float4 clipSpacePos = mul(viewPos, PassParams.MatProjection);
    
    output.pos = clipSpacePos.xyww;

   
    output.dir = normalize(input.pos);

    return output;
}
