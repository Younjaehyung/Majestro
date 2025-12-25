#include "params.hlsl"
#include "utils.hlsl"

// --------------
// Hull Shader
// --------------

struct VS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    
    uint instanceID : InstanceID;
};

struct PatchTess
{
    float edgeTess[3] : SV_TessFactor;
    float insideTess : SV_InsideTessFactor;
    
    uint instanceID : InstanceID;
};

struct HS_OUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    
    uint instanceID : InstanceID;
};

PatchTess ConstantHS(InputPatch<VS_OUT, 3> input, int patchID : SV_PrimitiveID)
{
    PatchTess output = (PatchTess) 0;


    output.instanceID = input[0].instanceID;

  
    uint idx = GlobalParams.BaseInstanceID + input[0].instanceID;
    RENDERPARAMS instance = InstanceParams[idx];


    float minDistance = PassParams.MinMaxTessDistance.x;
    float maxDistance = PassParams.MinMaxTessDistance.y;


    float3 edge0Pos = (input[1].pos + input[2].pos) * 0.5f;
    float3 edge1Pos = (input[2].pos + input[0].pos) * 0.5f;
    float3 edge2Pos = (input[0].pos + input[1].pos) * 0.5f;

    edge0Pos = mul(float4(edge0Pos, 1.f), Objects[instance.ObjectIndex].MatWorld).xyz;
    edge1Pos = mul(float4(edge1Pos, 1.f), Objects[instance.ObjectIndex].MatWorld).xyz;
    edge2Pos = mul(float4(edge2Pos, 1.f), Objects[instance.ObjectIndex].MatWorld).xyz;


    float3 cameraPos = PassParams.MatViewInv[3].xyz; 


    float edge0TessLevel = CalculateTessLevel(cameraPos, edge0Pos, minDistance, maxDistance, 4.f);
    float edge1TessLevel = CalculateTessLevel(cameraPos, edge1Pos, minDistance, maxDistance, 4.f);
    float edge2TessLevel = CalculateTessLevel(cameraPos, edge2Pos, minDistance, maxDistance, 4.f);


    output.edgeTess[0] = edge0TessLevel;
    output.edgeTess[1] = edge1TessLevel;
    output.edgeTess[2] = edge2TessLevel;


    float3 centerPos = (edge0Pos + edge1Pos + edge2Pos) / 3.0f;
    float centerTess = CalculateTessLevel(cameraPos, centerPos, minDistance, maxDistance, 4.f);
    output.insideTess = (edge0TessLevel + edge1TessLevel + edge2TessLevel + centerTess) * 0.25f;

    return output;
}


[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")] 
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
HS_OUT HS_Main(InputPatch<VS_OUT, 3> input,
               int vertexIdx : SV_OutputControlPointID,
               int patchID : SV_PrimitiveID)
{
    HS_OUT output = (HS_OUT) 0;

    output.pos = input[vertexIdx].pos;
    output.uv = input[vertexIdx].uv;

    output.instanceID = input[vertexIdx].instanceID;

    return output;
}

