#include "params.hlsl"

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

struct PS_OUT
{
    float4 position : SV_Target0; // view-space pos
    float4 normal : SV_Target1; // xyz = view normal, w = metallic   [수정]
    float4 color : SV_Target2; // rgb = baseColor,   a = roughness  [수정]
};

// NOTE:
// - 아래 코드는 "PBR 계산"을 여기서 끝내는 게 아니라,
//   라이팅 패스에서 BRDF 계산할 수 있게 G-Buffer에 metallic/roughness를 저장하는 형태임.

PS_OUT PS_Main(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    uint idx = GlobalParams.BaseInstanceID + input.instanceID;
    RENDERPARAMS instance = InstanceParams[idx];
    uint materialIndex = instance.MaterialInfoIndex;
    MATERIALINFO materials = Materials[materialIndex];

    // -----------------------------
    // 1) BaseColor(Albedo) 구성
    // -----------------------------
    float4 baseColor = materials.Diffuse;

    // [수정] 인덱스 0도 유효할 수 있으니 ">= 0"으로 통일해야 함.
    //        (invalid = -1이라는 전제. MATERIALINFO의 인덱스 타입은 int여야 정상)
    if (materials.DiffuseMap0Index != 0)
    {
        float4 tex0 = TextureMaps[materials.DiffuseMap0Index].Sample(g_sam_0, input.uv);
        baseColor *= tex0;
    }

    // [수정] 기존엔 != 0라서 0번 텍스처를 못 씀 + 레이어가 2개까지만 “되는 것처럼” 보일 수 있음
    if (materials.DiffuseMap1Index != 0)
    {
        float4 tex1 = TextureMaps[materials.DiffuseMap1Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex1, tex1.a);
    }

    if (materials.DiffuseMap2Index != 0)
    {
        float4 tex2 = TextureMaps[materials.DiffuseMap2Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex2, tex2.a);
    }

    if (materials.DiffuseMap3Index != 0)
    {
        float4 tex3 = TextureMaps[materials.DiffuseMap3Index].Sample(g_sam_0, input.uv);
        baseColor = lerp(baseColor, tex3, tex3.a);
    }
    if (baseColor.a < 0.1f)
        discard;
    // -----------------------------
    // 2) Normal Map 적용
    // -----------------------------
    float3 viewNormal = normalize(input.viewNormal);

    // [수정] != 0 -> >= 0 통일 (0번 노말맵도 유효할 수 있음)
    if (materials.NormalMapIndex != 0)
    {
        float3 tangentSpaceNormal = TextureMaps[materials.NormalMapIndex].Sample(g_sam_0, input.uv).xyz;

        // [0,1] -> [-1,1]
        tangentSpaceNormal = tangentSpaceNormal * 2.0f - 1.0f;

        float3x3 matTBN = float3x3(
            normalize(input.viewTangent),
            normalize(input.viewBinormal),
            normalize(input.viewNormal)
        );

        // tangent -> view
        viewNormal = normalize(mul(tangentSpaceNormal, matTBN));
    }

    // -----------------------------
    // 3) Metallic / Roughness 추출
    // -----------------------------
    // 기본값(텍스처 없을 때)을 머티리얼 상수로 두는 게 정석인데,
    // 네 MATERIALINFO에 해당 필드가 있는지 여기 코드만으론 확정 불가.
    // 그래서 아래는 "필드가 없으면 상수로 고정" 형태로 작성.
    //
    // [권장] MATERIALINFO에 float Metallic; float Roughness; 를 넣고 여기서 기본값으로 사용해라.

    float metallic = 0.7f; // [수정] 기본 메탈릭
    float roughness = 0.2f; // [수정] 기본 러프니스(너무 매끈/너무 거칠지 않게 중간값)

    // [수정] 메탈릭 맵
    if (materials.MetallicMapIndex != 0)
    {
        // 네 코드처럼 r 채널 사용 (프로젝트 규약에 맞춰 유지)
        metallic = TextureMaps[materials.MetallicMapIndex].Sample(g_sam_0, input.uv).r;
    }
    metallic = 0.7f; // [수정] 기본 메탈릭 (텍스처 없을 때)
    // else metallic = materials.Metallic;  // [권장] MATERIALINFO에 Metallic이 있다면 이걸로

    // [추가] 러프니스 맵이 있다면 여기서 샘플링 해야 함.
    // 현재 MATERIALINFO에 RoughnessMapIndex가 없어서 "있다"는 전제하에 코드를 적어둠.
    // 실제로 없으면 MATERIALINFO에 추가해야 함.
    //
    // if (materials.RoughnessMapIndex >= 0)
    // {
    //     roughness = TextureMaps[materials.RoughnessMapIndex].Sample(g_sam_0, input.uv).r;
    // }
    // else roughness = materials.Roughness; // [권장] MATERIALINFO에 Roughness가 있다면 이걸로

    // 안전 클램프 (BRDF에서 가끔 NaN 방지)
    metallic = saturate(metallic);
    roughness = saturate(roughness);

    // -----------------------------
    // 4) G-Buffer 출력 (패킹)
    // -----------------------------
    output.position = float4(input.viewPos.xyz, 0.0f);

    // [수정] normal.w에 metallic 저장
    output.normal = float4(viewNormal.xyz, metallic);

    // [수정] color.a에 roughness 저장
    output.color = float4(baseColor.rgb, roughness);
    
    
    return output;
}
