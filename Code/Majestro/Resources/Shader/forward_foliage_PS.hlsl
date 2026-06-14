
#define FORWARD_PLUS_TILE_SIZE           16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128

#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos          : SV_Position;
    float2 uv           : TEXCOORD;
    float3 viewPos      : POSITION;
    float3 viewNormal   : NORMAL;
    float3 viewTangent  : TANGENT;
    float3 viewBinormal : BINORMAL;
    uint   instanceID   : InstanceID;
};

float4 PS_Main(VS_OUT input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    const uint idx          = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS inst = InstanceParams[idx];
    const MATERIALINFO mtl  = Materials[inst.MaterialInfoIndex];


    float4 baseColor = mtl.Diffuse;
    if (mtl.DiffuseMap0Index >= 0)
        baseColor *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv);
    if (mtl.DiffuseMap1Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, t, t.a);
    }
    if (mtl.DiffuseMap2Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, t, t.a);
    }
    if (mtl.DiffuseMap3Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, t, t.a);
    }


    
    if (baseColor.a < 0.001f)
        clip(-1);
   
    
    float3 viewNormal = normalize(mul(-Lights[0].direction, PassParams.MatView).xyz);

    if (mtl.NormalMapIndex >= 0)
    {
        float3 n = TextureMaps[mtl.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;
        n = n * 2.0f - 1.0f;
        float3x3 tbn = float3x3(
            normalize(input.viewTangent),
            normalize(input.viewBinormal),
            normalize(input.viewNormal));
        viewNormal = normalize(mul(n, tbn));
    }

    float metallic  = mtl.Metallic;
    float roughness = mtl.Roughness;
    if (mtl.MetallicMapIndex >= 0)
        metallic  = TextureMaps[mtl.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    // RoughnessMap 우선, 없으면 SpecularcMap 슬롯(FBX Roughness 채널) 폴백
    if (mtl.RoughnessMapIndex >= 0)
        roughness = TextureMaps[mtl.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    else if (mtl.SpecularcMapIndex >= 0)
        roughness = TextureMaps[mtl.SpecularcMapIndex].Sample(g_sam_0, input.uv).r;
    metallic  = saturate(metallic);
    roughness = saturate(roughness);

    // Forward+ 타일 인덱스 계산
    const uint2 pixelCoord = uint2(input.pos.xy);
    const uint  tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
    const uint2 tileCoord  = pixelCoord / FORWARD_PLUS_TILE_SIZE;
    const uint  tileIndex  = tileCoord.y * tileCountX + tileCoord.x;
    const uint2 tileMeta   = ForwardPlusTileMeta[tileIndex];


    float3 totalDiffuse  = 0.0f;
    float3 totalSpecular = 0.0f;
    bool   shadowApplied = false;

    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];

        LightColor lc = CalculateLightColorPBR(
            lightIndex, viewNormal, input.viewPos,
            baseColor.rgb, metallic, roughness);

        float3 diff = lc.diffuse.rgb;
        float3 spec = lc.specular.rgb;


        if (Lights[lightIndex].lightType == 0 && !shadowApplied)
        {
            float vis = CalculateCSMShadow(
                input.viewPos, viewNormal, Lights[lightIndex].direction.xyz);
            diff *= vis;
            spec *= vis;
            shadowApplied = true;
        }

        totalDiffuse  += diff;
        totalSpecular += spec;
    }

   
    float3 V = normalize(-input.viewPos);
    IBLResult ibl = CalculateIBLAmbient(
        viewNormal, V, baseColor.rgb, metallic, roughness);


    // 풀은 specular 반사가 없음
    float3 finalRGB = baseColor.rgb * (totalDiffuse + ibl.diffuse)
                    + totalSpecular;

    //return float4(finalRGB, 1,0f);
    return float4(finalRGB, baseColor.a);
}
