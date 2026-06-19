#include "params.hlsl"

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34f, 456.21f));
    p += dot(p, p + 45.32f);
    return frac(p.x * p.y);
}

void BlendBuffIcon(
    inout float3 color,
    float2 screenUv,
    int atlasTexIdx,
    float2 atlasCell,
    float strength,
    float seed,
    float time)
{
    if (atlasTexIdx < 0 || strength <= 0.001f)
        return;

    float baseOffset = Hash21(float2(seed * 2.31f, seed + 4.17f));
    float cycleTime = time + baseOffset;
    float cycleIndex = floor(cycleTime);
    float progress = frac(cycleTime);

    // 수정사항: 현재 반복 주기를 난수 시드에 포함해 매번 다른 배치를 만든다.
    // 같은 아이콘 슬롯도 재생될 때마다 위치, 방향, 크기, 이동량이 달라진다.
    float cycleSeed = seed + cycleIndex * 17.37f;
    float randomA = Hash21(float2(cycleSeed, cycleSeed * 1.73f));
    float randomB = Hash21(float2(cycleSeed * 2.31f, cycleSeed + 4.17f));
    float randomC = Hash21(float2(cycleSeed * 0.83f + 6.4f, cycleSeed * 3.17f));
    float randomD = Hash21(float2(cycleSeed * 4.13f + 2.8f, cycleSeed * 0.41f));

    // 수정사항: 아이콘 중심을 화면 가장자리에 더 가깝게 배치한다.
    bool useRightSide = randomD > 0.5f;
    float edgeX = lerp(0.015f, 0.09f, randomA);
    float centerX = useRightSide ? 1.0f - edgeX : edgeX;
    centerX += sin(time * lerp(1.2f, 2.1f, randomC) + cycleSeed * 2.4f) * 0.008f;

    // 수정사항: 화면 아래에서 시작하지 않고 좌우 외곽의 임의 높이에서 생성한다.
    float spawnY = lerp(0.10f, 0.90f, randomC);

    // 수정사항: 1초 동안 화면 전체를 통과하지 않고 생성 위치에서 위로 조금만 이동한다.
    float centerY = spawnY - progress * lerp(0.025f, 0.080f, randomD);

    // 수정사항: 아이콘 수를 줄여도 효과가 분명하게 보이도록 크기를 더 키운다.
    float iconPixels = lerp(60.0f, 96.0f, randomB);
    float2 iconSizeUv = iconPixels / max(PassParams.ScreenSize.xy, float2(1.0f, 1.0f));
    float2 iconUv = (screenUv - float2(centerX, centerY)) / iconSizeUv + 0.5f;

    if (any(iconUv < 0.0f) || any(iconUv > 1.0f))
        return;

    // UI_Effect_Sheet는 화살표, 회복, 방패, 공격 순서의 2행 2열 시트다.
    float2 atlasUv = (atlasCell + iconUv) * 0.5f;
    float4 iconSample = TextureMaps[atlasTexIdx].Sample(g_sam_0, atlasUv);

    // 수정사항: 페이드 시작 시점도 매 주기마다 달라 반복되는 박자를 줄인다.
    float fadeInEnd = lerp(0.06f, 0.16f, randomA);
    float fadeOutStart = lerp(0.38f, 0.68f, randomC);
    float lifeFade = smoothstep(0.0f, fadeInEnd, progress);
    lifeFade *= 1.0f - smoothstep(fadeOutStart, 1.0f, progress);
    float pulse = 0.82f + 0.18f * sin(time * lerp(2.4f, 3.8f, randomD) + cycleSeed);
    float alpha = saturate(iconSample.a * strength * lifeFade * pulse);

    color = lerp(color, iconSample.rgb, alpha * 0.92f);
    color += iconSample.rgb * alpha * 0.12f;
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    uint srcIdx = (uint)data.PreviousStep;
    float time = data.ExtValue[0].x;
    float lowStrength = saturate(data.ExtValue[0].y);
    float healStrength = saturate(data.ExtValue[0].z);
    float noiseTiling = max(data.ExtValue[0].w, 0.001f);
    int noiseTexIdx = data.ExtTex[0];
    int atlasTexIdx = data.ExtTex[1];

    float4 buffStrengths = saturate(data.ExtValue[3]);
    float healIconStrength = buffStrengths.x;
    float moveIconStrength = buffStrengths.y;
    float shieldIconStrength = buffStrengths.z;
    float attackIconStrength = buffStrengths.w;

    float effectStrength = saturate(max(
        max(lowStrength, healStrength),
        max(max(healIconStrength, moveIconStrength), max(shieldIconStrength, attackIconStrength))));

    float4 color = Gbuffer[srcIdx].Sample(g_sam_0, input.uv);
    if (effectStrength <= 0.0001f)
        return color;

    float2 centered = input.uv - float2(0.5f, 0.5f);
    float aspect = max(PassParams.ScreenSize.x / max(PassParams.ScreenSize.y, 1.0f), 0.0001f);
    centered.x *= aspect;

    float dist = length(centered);
    float angle = atan2(centered.y, centered.x);
    float edgeMask = smoothstep(0.42f, 0.90f, dist);

    float waveA = sin(time * 2.7f + dist * 18.0f + angle * 3.0f);
    float waveB = sin(time * 1.6f - dist * 12.6f + angle * 7.0f);
    float wave = 0.5f + 0.5f * (waveA * 0.65f + waveB * 0.35f);
    float textureDirty = 0.0f;

    if (noiseTexIdx >= 0)
    {
        float2 noiseUv = input.uv * noiseTiling + float2(time * 0.035f, -time * 0.02f);
        float4 noiseSample = TextureMaps[noiseTexIdx].Sample(g_sam_0, frac(noiseUv));
        textureDirty = dot(noiseSample.rgb, float3(0.3333f, 0.3333f, 0.3334f));
    }
    else
    {
        float timeCell = floor(time * 7.0f);
        float2 proceduralCell = floor(
            (input.uv + float2(time * 0.025f, -time * 0.018f)) * float2(26.0f, 15.0f));
        textureDirty = Hash21(proceduralCell + timeCell);
    }

    float dirtyPatch = lerp(0.72f, 1.34f, textureDirty);
    float dirtyCut = smoothstep(0.22f, 0.88f, textureDirty);
    float animatedMask = edgeMask * lerp(0.74f, 1.26f, wave);
    animatedMask *= lerp(0.82f, dirtyPatch, 0.55f);
    animatedMask *= lerp(0.88f, 1.18f, dirtyCut);
    animatedMask = saturate(animatedMask);

    float lowPulse = 0.78f + 0.22f * sin(time * data.ExtValue[1].w);
    float healPulse = 0.70f + 0.30f * sin(time * data.ExtValue[2].w + dist * 8.0f);
    float lowMask = animatedMask * lowStrength * lowPulse;
    float healMask = animatedMask * healStrength * healPulse;

    float3 dirtyLowColor = lerp(float3(0.95f, 0.005f, 0.0f), data.ExtValue[1].rgb, textureDirty);
    float3 dirtyHealColor = lerp(float3(0.18f, 0.80f, 0.04f), data.ExtValue[2].rgb, textureDirty);
    float lowEdge = saturate(lowMask * 1.25f);

    color.rgb *= 1.0f - lowEdge * 0.28f;
    color.g *= 1.0f - lowEdge * 0.42f;
    color.b *= 1.0f - lowEdge * 0.50f;
    color.rgb += dirtyLowColor * lowEdge * 1.25f;
    color.rgb += dirtyHealColor * healMask * 0.85f;

    // 수정사항: 버프 비네팅은 기존 빈사 및 회복 비네팅과 별도의 테두리 마스크를 사용한다.
    // 화면 중심까지 번지지 않고 가장자리 약 12퍼센트 영역에서만 강하게 나타난다.
    float distanceToBorder = min(
        min(input.uv.x, 1.0f - input.uv.x),
        min(input.uv.y, 1.0f - input.uv.y));
    float buffBorderMask = 1.0f - smoothstep(0.015f, 0.12f, distanceToBorder);
    buffBorderMask = pow(saturate(buffBorderMask), 1.35f);

    // 노이즈와 느린 맥동을 더해 테두리가 단색 띠처럼 고정되어 보이지 않게 한다.
    float buffBorderPulse = 0.82f + 0.18f * sin(time * 2.2f);
    float buffBorderNoise = lerp(0.82f, 1.15f, textureDirty);
    float buffBorderStrength = buffBorderMask * buffBorderPulse * buffBorderNoise;

    color.rgb += float3(0.20f, 0.85f, 0.22f) * buffBorderStrength * healIconStrength * 0.30f;
    color.rgb += float3(0.15f, 0.48f, 1.00f) * buffBorderStrength * moveIconStrength * 0.30f;
    color.rgb += float3(0.50f, 0.70f, 0.92f) * buffBorderStrength * shieldIconStrength * 0.27f;
    color.rgb += float3(1.00f, 0.18f, 0.12f) * buffBorderStrength * attackIconStrength * 0.30f;

    // 수정사항: 한 효과당 아이콘 개수를 다섯 개에서 세 개로 줄인다.
    [unroll]
    for (int iconIndex = 0; iconIndex < 3; ++iconIndex)
    {
        float iconSeed = (float)iconIndex;
        BlendBuffIcon(color.rgb, input.uv, atlasTexIdx, float2(1.0f, 0.0f), healIconStrength, iconSeed + 10.0f, time);
        BlendBuffIcon(color.rgb, input.uv, atlasTexIdx, float2(0.0f, 0.0f), moveIconStrength, iconSeed + 20.0f, time);
        BlendBuffIcon(color.rgb, input.uv, atlasTexIdx, float2(0.0f, 1.0f), shieldIconStrength, iconSeed + 30.0f, time);
        BlendBuffIcon(color.rgb, input.uv, atlasTexIdx, float2(1.0f, 1.0f), attackIconStrength, iconSeed + 40.0f, time);
    }

    return color;
}
