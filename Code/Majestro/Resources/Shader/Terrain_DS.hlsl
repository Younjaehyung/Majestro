#include "params.hlsl"
#include "utils.hlsl"


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


// --------------
// Domain Shader
// --------------

struct DS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float2 fulluv : TEXCOORD1;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;
    float3 viewBinormal : BINORMAL;
    
    uint instanceID : InstanceID;
};

[domain("tri")]
DS_OUT DS_Main(const OutputPatch<HS_OUT, 3> input, float3 location : SV_DomainLocation, PatchTess patch)
{
    
    
    DS_OUT output = (DS_OUT) 0.f;

   
    output.instanceID = input[0].instanceID;
    uint idx = GlobalParams.BaseInstanceID + input[0].instanceID;;
    RENDERPARAMS instance = InstanceParams[idx];
    MATERIALINFO material = Materials[instance.MaterialInfoIndex];
    
    matrix WV = mul(Objects[instance.ObjectIndex].MatWorld, PassParams.MatView);
    matrix WVP = mul(WV, PassParams.MatProjection);
    

    
    float3 localPos = input[0].pos * location[0] + input[1].pos * location[1] + input[2].pos * location[2];
    float2 uv = input[0].uv * location[0] + input[1].uv * location[1] + input[2].uv * location[2];

    int tileCountX = PassParams.TileCountX;
    int tileCountZ = PassParams.TileCountZ;
    int mapWidth = PassParams.HeightMapResolution.x;
    int mapHeight = PassParams.HeightMapResolution.y;

    float2 fullUV = float2(uv.x / (float) tileCountX, uv.y / (float) tileCountZ);
    float height = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, fullUV, 0).x;


    //localPos.y = height;
    localPos.y = (height - 0.5f) * 512.0f;
    
    float2 deltaUV = float2(1.f / mapWidth, 1.f / mapHeight);
    float2 deltaPos = float2(tileCountX * deltaUV.x, tileCountZ * deltaUV.y);

    //float upHeight = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x, fullUV.y - deltaUV.y), 0).x;
    //float downHeight = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x, fullUV.y + deltaUV.y), 0).x;
    //float rightHeight = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x + deltaUV.x, fullUV.y), 0).x;
    //float leftHeight = TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x - deltaUV.x, fullUV.y), 0).x;

    float upHeight = (TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x, fullUV.y - deltaUV.y), 0).x - 0.5f) * 512.0f;
    float downHeight = (TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x, fullUV.y + deltaUV.y), 0).x - 0.5f) * 512.0f;
    float rightHeight = (TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x + deltaUV.x, fullUV.y), 0).x - 0.5f) * 512.0f;
    float leftHeight = (TextureMaps[material.DiffuseMap2Index].SampleLevel(g_sam_Terrain, float2(fullUV.x - deltaUV.x, fullUV.y), 0).x - 0.5f) * 512.0f;
    
    float3 localTangent = float3(localPos.x + deltaPos.x, rightHeight, localPos.z) - float3(localPos.x - deltaPos.x, leftHeight, localPos.z);
    float3 localBinormal = float3(localPos.x, upHeight, localPos.z + deltaPos.y) - float3(localPos.x, downHeight, localPos.z - deltaPos.y);
   
    output.pos = mul(float4(localPos, 1.f), WVP);
    output.viewPos = mul(float4(localPos, 1.f), WV).xyz;

    output.viewTangent = normalize(mul(float4(localTangent, 0.f), WV)).xyz;
    output.viewBinormal = normalize(mul(float4(localBinormal, 0.f), WV)).xyz;
    output.viewNormal = normalize(cross(output.viewBinormal, output.viewTangent));

    output.fulluv = fullUV;
    output.uv = uv;

    return output;
}

