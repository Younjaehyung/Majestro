
#include "params.hlsli"
#include "utils.hlsli"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VS_OUT
{
    float4 pos : SV_Position;   //문법상 고정된 이름임.
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;   //T
    float3 viewBinormal : BINORMAL; //B
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT)0;

    uint objectIndex = GlobalParams.ObjectIndex;
    
    
    output.pos = mul(float4(input.pos, 1.f), Objects[objectIndex].MatWorld);
    output.uv = input.uv;

    matrix WV = mul(output.pos, PassParams.MatView);
    
    output.viewPos = mul(float4(input.pos, 1.f), WV).xyz;
    output.viewNormal = normalize(mul(float4(input.normal, 0.f), WV).xyz);
    output.viewTangent = normalize(mul(float4(input.tangent, 0.f), WV).xyz);
    output.viewBinormal = normalize(cross(output.viewTangent, output.viewNormal));
    
    return output;
}


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

VS_TEX_OUT VS_Tex(VS_TEX_IN input)
{
    VS_TEX_OUT output = (VS_TEX_OUT) 0;

    uint objectIndex = GlobalParams.ObjectIndex;
    matrix view = PassParams.MatView;
    matrix projection = PassParams.MatProjection;
    matrix WVP = mul(mul(Objects[objectIndex].MatWorld, view), projection);
    
    output.pos = mul(float4(input.pos, 1.f), WVP);
    output.uv = input.uv;

    return output;
}
