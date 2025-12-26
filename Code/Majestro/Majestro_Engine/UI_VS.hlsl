
#include "params.hlsl"
#include "utils.hlsl"


//// [Texture Shader] 텍스쳐가 라이팅의 영향을 안받게 하기 위한 쉐이더코드
//// g_tex_0 : Output Texture
//// AlphaBlend : true
//struct VS_TEX_IN
//{
//    float3 pos : POSITION;
//    float2 uv : TEXCOORD;
//};

//struct VS_TEX_OUT
//{
//    float4 pos : SV_Position;
//    float2 uv : TEXCOORD;
//};

//// uint Index0 = ObjectIndex;
//// uint Index1 = MaterialInfoIndex;

//VS_TEX_OUT VS_Main(VS_TEX_IN input)
//{
//    VS_TEX_OUT output = (VS_TEX_OUT) 0;

//    uint idx = GlobalParams.BaseInstanceID;
//    RENDERPARAMS instance = InstanceParams[idx];
//    uint objectIndex = instance.ObjectIndex;
//    matrix view = PassParams.MatView;
//    matrix projection = PassParams.MatProjection;
//    matrix WVP = mul(mul(Objects[objectIndex].MatWorld, view), projection);
//    matrix VP = mul(view, projection);
    
//    output.pos = mul(float4(input.pos, 1.f), VP);
//    output.uv = input.uv;

   
//    return output;
//}


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

    // 픽셀 좌표 → NDC
    float2 ndc;
    ndc.x = (input.pos.x / PassParams.ScreenSize.x) * 2.0f - 1.0f;
    ndc.y = 1.0f - (input.pos.y / PassParams.ScreenSize.y) * 2.0f; // y축 반전 (상단이 0이니까)

    output.pos = float4(ndc.xy, 0.0f, 1.0f);
    output.uv = input.uv;

    return output;
}
