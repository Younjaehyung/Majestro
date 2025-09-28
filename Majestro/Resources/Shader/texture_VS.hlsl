
#include "params.hlsl"
#include "utils.hlsl"


// [Texture Shader] 텍스쳐가 라이팅의 영향을 안받게 하기 위한 쉐이더코드
// g_tex_0 : Output Texture
// AlphaBlend : true
struct VS_TEX_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_TEX_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

// uint Index0 = ObjectIndex;
// uint Index1 = MaterialInfoIndex;

VS_TEX_OUT VS_Tex(VS_TEX_IN input)
{
    VS_TEX_OUT output = (VS_TEX_OUT) 0;

    uint objectIndex = GlobalParams.Index0;
    matrix view = PassParams.MatView;
    matrix projection = PassParams.MatProjection;
    matrix WVP = mul(mul(Objects[objectIndex].MatWorld, view), projection);
    
    output.pos = mul(float4(input.pos, 1.f), WVP);
    output.uv = input.uv;

    return output;
}
