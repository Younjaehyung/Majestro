#include "params.hlsl"
#include "utils.hlsl"


// --------------
// Vertex Shader
// --------------

struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = input;

    return output;
}

// --------------
// Hull Shader
// --------------

struct PatchTess
{
    float edgeTess[3] : SV_TessFactor;
    float insideTess : SV_InsideTessFactor;
};

struct HS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

// Constant HS
PatchTess ConstantHS(InputPatch<VS_OUT, 3> input, int patchID : SV_PrimitiveID)
{
    PatchTess output = (PatchTess) 0.f;

    output.edgeTess[0] = 1;
    output.edgeTess[1] = 2;
    output.edgeTess[2] = 3;
    output.insideTess = 1;

    return output;
}

// Control Point HS
[domain("tri")] // ��ġ�� ���� (tri, quad, isoline)
[partitioning("integer")] // subdivision mode (integer �Ҽ��� ����, fractional_even, fractional_odd)
[outputtopology("triangle_cw")] // (triangle_cw, triangle_ccw, line)
[outputcontrolpoints(3)] // �ϳ��� �Է� ��ġ�� ����, HS�� ����� ������ ����
[patchconstantfunc("ConstantHS")] // ConstantHS �Լ� �̸�
HS_OUT HS_Main(InputPatch<VS_OUT, 3> input, int vertexIdx : SV_OutputControlPointID, int patchID : SV_PrimitiveID)
{
    HS_OUT output = (HS_OUT) 0.f;

    output.pos = input[vertexIdx].pos;
    output.uv = input[vertexIdx].uv;

    return output;
}