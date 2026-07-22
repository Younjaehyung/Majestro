#include "params.hlsl"
#include "utils.hlsl"

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float4 weight : BONEWEIGHT;
    float4 indices : BONEINDICES;

    uint instanceID : SV_InstanceID;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    nointerpolation float objectAlpha : TEXCOORD0; // 카메라 페이드 디더용
    float2 uv : TEXCOORD1;                          // 디졸브 노이즈용
    nointerpolation float dissolve : TEXCOORD2;     // 디졸브 진행도 (Extra.w)
    nointerpolation int noiseTexIdx : TEXCOORD3;    // 디졸브 노이즈 텍스처 인덱스 (ExtTex[2], -1=절차적)
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];

    matrix WV  = mul(Objects[instance.ObjectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);

    if (instance.LightIndex >= 0)
        Skinning(input.pos, input.normal, input.tangent, input.weight, input.indices, AnimInstance[instance.LightIndex].ReulstIndex);

    output.pos = mul(float4(input.pos, 1.f), WVP);
    output.objectAlpha = Objects[instance.ObjectIndex].Extra1.x;
    output.uv = input.uv;
    output.dissolve = Objects[instance.ObjectIndex].Extra1.w;
    output.noiseTexIdx = Materials[instance.MaterialInfoIndex].ExtTex[2];

    return output;
}
