#include "params.hlsl"

//Texture2D SceneLit : register(t0); // 조명 포함 최종 컬러
//Texture2D SceneAlbedo : register(t1); // BaseColor/Albedo (조명 없는 색)
//Texture2D SceneDepth : register(t2); // 뎁스 (linear or device depth)


//cbuffer ToonCB : register(b0)
//{
//   // float2 invScreenSize;
//   // float edge0; // 예: 0.4
//  //  float edge1; // 예: 0.6
// //   float3 shadowTint; // 그림자 틴트
//  //  float3 lightTint; // 밝은 틴트
// //   float depthThreshold; // 예: 아주 먼 거리 기준
//   // float litAddScale; // 글의 "PostProcessInput0 * 0.5" 같은 보정 스케일
//   // float eps; // 1e-4 정도
//};

float Luma(float3 c)
{
    return dot(c, float3(0.299, 0.587, 0.114));
}

struct VSOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 PS_Main(VSOut i) : SV_Target
{
    float3 lit = Gbuffer[0].Sample(g_sam_0, i.uv).rgb;
    float3 albedo = Gbuffer[3].Sample(g_sam_0, i.uv).rgb;

    // 1) Desaturation ~ Luma
    float litGray = Luma(lit);
    float albedoGray = max(Luma(albedo), 1e-4);

    // 2) 라이팅 정보 추출: (lit / unlit)
    float lighting = saturate(litGray / albedoGray);

    // 3) 계단화(부드러운 경계)
    float toon = smoothstep(0.4, 0.6, lighting);

    // 4) Shadow/Light 틴트
    float3 tint = lerp(0, 1, toon);

    // 5) 실제 색 입히기 + (lit 보정 더하기)  ← 글의 의도와 동일 :contentReference[oaicite:7]{index=7}
    float3 toonColor = tint * albedo + lit * 0.5;

    // 6) 거리 기반으로 멀면 원본으로 복귀 :contentReference[oaicite:8]{index=8}
    float depth = Gbuffer[1].Sample(g_sam_0, i.uv).r;
    float farMask = step(300, depth); // depth가 linear인지 주의
    float3 finalColor = lerp(toonColor, lit, farMask);

    return float4(finalColor, 1.0);
}