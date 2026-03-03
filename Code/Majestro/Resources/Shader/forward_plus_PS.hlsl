#include "params.hlsl"
#include "utils.hlsl"

#define FORWARD_PLUS_TILE_SIZE 16
#define FORWARD_PLUS_MAX_LIGHTS_PER_TILE 128

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 viewPos : POSITION;
    float3 viewNormal : NORMAL;
    float3 viewTangent : TANGENT;
    float3 viewBinormal : BINORMAL;
    uint instanceID : InstanceID;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    const uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS instance = InstanceParams[idx];
    const MATERIALINFO mtl = Materials[instance.MaterialInfoIndex];

    float4 color = mtl.Diffuse;

    // deferred/forward와 동일하게 0~3 모두 반영
    if (mtl.DiffuseMap0Index >= 0)
        color *= TextureMaps[mtl.DiffuseMap0Index].Sample(g_sam_0, input.uv);

    if (mtl.DiffuseMap1Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (mtl.DiffuseMap2Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (mtl.DiffuseMap3Index >= 0)
    {
        float4 t = TextureMaps[mtl.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        color = lerp(color, t, t.a);
    }
    if (color.a < 0.01f)
        discard;

    float3 viewNormal = normalize(input.viewNormal);
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
    const uint2 pixelCoord = uint2(input.pos.xy);
    const uint tileCountX = (uint) ceil(PassParams.ScreenSize.x / FORWARD_PLUS_TILE_SIZE);
    const uint2 tileCoord = pixelCoord / FORWARD_PLUS_TILE_SIZE;
    const uint tileIndex = tileCoord.y * tileCountX + tileCoord.x;
    const uint2 tileMeta = ForwardPlusTileMeta[tileIndex];

    LightColor totalColor = (LightColor) 0.f;
    bool directionalShadowApplied = false;

    [loop]
    for (uint i = 0; i < tileMeta.y && i < FORWARD_PLUS_MAX_LIGHTS_PER_TILE; ++i)
    {
        const uint lightIndex = ForwardPlusLightIndices[tileMeta.x + i];
        const LightColor lightColor = CalculateLightColorPBR(lightIndex, viewNormal, input.viewPos, color.rgb, 0.0f, 0.0f);
        totalColor.diffuse += lightColor.diffuse;
        totalColor.ambient += lightColor.ambient;
        totalColor.specular += lightColor.specular;

        if (!directionalShadowApplied && Lights[lightIndex].lightType == 0)
        {
            const float visibility = CalculateCSMShadow(input.viewPos, viewNormal, Lights[lightIndex].direction.xyz);
            totalColor.diffuse *= visibility;
            totalColor.ambient *= visibility;
            directionalShadowApplied = true;
        }
    }

    color.xyz = (totalColor.diffuse.xyz * color.xyz)
              + (totalColor.ambient.xyz * color.xyz)
              + totalColor.specular.xyz;

    //color.xyz = floor(saturate(color.xyz) * 4.0f) / 4.0f;
    return color;
}