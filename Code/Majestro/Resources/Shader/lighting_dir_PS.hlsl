#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    uint instanceID : InstanceID;
};

struct PS_OUT
{
    float4 diffuse : SV_Target0;
    float4 specular : SV_Target1;
};

// [정리: 네 코드 기준 Gbuffer 배열 사용]
// Gbuffer[0] : Shadow depth
// Gbuffer[1] : Position (view-space xyz)
// Gbuffer[2] : Normal+Metallic (xyz = view normal, w = metallic)   // [수정 반영]
// Gbuffer[3] : Albedo+Roughness (rgb = baseColor, a = roughness)   // [추가 필요]

PS_OUT PS_DirLight(VS_OUT input)
{
    PS_OUT output = (PS_OUT) 0;

    // 라이트 선택
    int index = 0; // Instance.LightIndex; (네 엔진 로직에 맞게 사용)
    LIGHTINFO light = Lights[index];

    // -----------------------------
    // [수정] G-Buffer 샘플링 확장
    // -----------------------------
    float3 viewPos = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;

    if (viewPos.z <= 0.f)
        clip(-1);

    float4 n_m = Gbuffer[2].Sample(g_sam_0, input.uv);
    float3 viewNormal = normalize(n_m.xyz);
    float metallic = saturate(n_m.w); // [수정] normal.w에서 metallic 읽기

    float4 a_r = Gbuffer[3].Sample(g_sam_0, input.uv); // [추가] Albedo(RGB)+Roughness(A)
    float3 baseColor = a_r.rgb;
    float roughness = saturate(a_r.a);


    LightColor color = CalculateLightColorPBR(index, viewNormal, viewPos, baseColor, metallic, roughness);


    if (length(color.diffuse.rgb) != 0)
    {
        matrix shadowCameraVP = mul(light.MatView, light.MatProjection);

        float4 worldPos = mul(float4(viewPos.xyz, 1.f), light.MatViewInv);
        float4 shadowClipPos = mul(worldPos, shadowCameraVP);
        float depth = shadowClipPos.z / shadowClipPos.w;

        float2 uv = shadowClipPos.xy / shadowClipPos.w;
        uv.y = -uv.y;
        uv = uv * 0.5 + 0.5;

        if (0 < uv.x && uv.x < 1 && 0 < uv.y && uv.y < 1)
        {
            float shadowDepth = Gbuffer[0].Sample(g_sam_0, uv).x;

            // [권장] 바이어스는 상수보다 N·L 기반으로 키우는게 좋지만, 기존 유지
            if (shadowDepth > 0 && depth > shadowDepth + 0.00001f)
            {
                // [수정] PBR에서도 shadow는 diffuse/specular 모두에 영향
                color.diffuse *= 0.5f;
                color.specular *= 0.0f;
            }
        }
    }

    // 출력 (너는 diffuse/ambient를 한 버퍼에 합치고 specular는 별도 누적)
    output.diffuse = color.diffuse + color.ambient * 2.0f;
    output.specular = color.specular;

    return output;
}
