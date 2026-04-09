#include "params.hlsl"
#include "utils.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float4 PS_Main(VS_OUT input) : SV_TARGET
{
    float2 uv = input.uv;

    // PRE_DEPTH에서 깊이값 읽기
    float depth = Gbuffer[9].Sample(g_sam_0, uv).r;

    // 배경(sky)은 velocity 0
    if (depth >= 1.0f)
        return float4(0.f, 0.f, 0.f, 1.f);

    // UV  -> NDC (DX 좌표계: Y 반전)
    float2 ndcXY = uv * float2(2.f, -2.f) + float2(-1.f, 1.f);
    float4 ndcPos = float4(ndcXY, depth, 1.f);

    
    // NDC  -> 뷰 공간 (Projection 역변환)
    float4 viewPos = mul(ndcPos, PassParams.MatProjectionInv);
    viewPos /= viewPos.w;

    // 뷰 공간  -> 월드 공간 (View 역변환)
    float4 worldPos = mul(viewPos, PassParams.MatViewInv);

    // 월드 공간 -> 이전 프레임 클립 공간
    float4 prevView = mul(worldPos, PassParams.MatPrevView);
    float4 prevClip = mul(prevView, PassParams.MatPrevProjection);

    // 이전 프레임 UV 계산 (DX 좌표계 Y 반전 복원)
    float2 prevNDC = prevClip.xy / prevClip.w;
    float2 prevUV  = prevNDC * float2(0.5f, -0.5f) + 0.5f;

    // velocity = 현재 UV - 이전 UV
    float2 velocity = uv - prevUV;

    //    float2 screenCenter = float2(0.5f, 0.5f);
    //float2 radialDir = normalize(uv - screenCenter + 0.0001f);
    //float radialStrength = 0.008f; // 이 값으로 중앙 blur 강도 조절
    //velocity += radialDir * radialStrength;
    
    return float4(velocity, 0.f, 1.f);
}
