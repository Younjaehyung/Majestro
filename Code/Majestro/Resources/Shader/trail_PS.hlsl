#include "params.hlsl"

struct VS_OUT
{
    float4 pos              : SV_Position;
    float2 uv               : TEXCOORD0;
    float3 color            : COLOR0;
    float alpha             : TEXCOORD1;
    float textureIndex      : TEXCOORD2;
    float textureOption     : TEXCOORD3;
};

float4 PS_Main(VS_OUT input) : SV_Target
{
    // 수정: ribbon 양쪽 가장자리는 부드럽게 투명 처리하고, 최신 구간은 약간 밝게 해 휘두른 방향을 읽기 쉽게 만든다.
    float edge = 1.f - abs(input.uv.x * 2.f - 1.f);
    edge = saturate(edge);
    edge *= edge;

    float headBoost = lerp(0.85f, 1.2f, saturate(input.uv.y));
    float alpha = saturate(input.alpha * edge);
    float3 color = input.color * headBoost;

    int textureIndex = (int)round(input.textureIndex);
    if (textureIndex >= 0)
    {
        // 수정: WeaponTrailComponent.mTextureName으로 지정한 Texture 리소스를 trail 마스크/색상으로 샘플링한다.
        // textureOption이 음수면 텍스처 RGB까지 사용하고, 양수면 텍스처 alpha/RGB 밝기만 기존 trail 색상에 곱한다.
        float4 trailTex = TextureMaps[textureIndex].Sample(g_sam_0, input.uv);
        float textureMask = max(trailTex.a, max(trailTex.r, max(trailTex.g, trailTex.b)));
        float textureWeight = saturate(abs(input.textureOption));
        alpha *= lerp(1.f, textureMask, textureWeight);

        if (input.textureOption < 0.f)
            color *= trailTex.rgb;
    }

    return float4(color, alpha);
}
