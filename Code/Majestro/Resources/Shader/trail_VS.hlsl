#include "params.hlsl"

struct VS_IN
{
    float3 pos      : POSITION;
    float2 uv       : TEXCOORD;
    float3 color    : NORMAL;
    float3 tangent  : TANGENT;
    float4 weight   : BONEWEIGHT;
    float4 indices  : BONEINDICES;
};

struct VS_OUT
{
    float4 pos              : SV_Position;
    float2 uv               : TEXCOORD0;
    float3 color            : COLOR0;
    float alpha             : TEXCOORD1;
    float textureIndex      : TEXCOORD2;
    float textureOption     : TEXCOORD3;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    // 수정: TrailPass가 월드 공간 ribbon 정점을 업로드하므로 여기서는 카메라 View/Projection만 적용한다.
    matrix viewProj = mul(PassParams.MatView, PassParams.MatProjection);
    output.pos = mul(float4(input.pos, 1.f), viewProj);
    output.uv = input.uv;
    output.color = input.color;
    output.alpha = input.tangent.x;
    // 수정: TrailPass가 공용 Vertex.tangent의 남는 채널에 넣은 리소스 인덱스와 텍스처 옵션을 픽셀 셰이더로 전달한다.
    output.textureIndex = input.tangent.y;
    output.textureOption = input.tangent.z;
    return output;
}
