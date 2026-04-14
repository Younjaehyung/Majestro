#include "params.hlsl"
#include "math.hlsl"


// 핵심 설계: 법선은 elevation 계산에 쓰지 않음
//   - dot(normalize(diff), normalVS) 방식은 폴리곤 경계에서 법선이
//     불연속 -> AO 가 폴리곤 경계를 따라 잘림 (폴리곤 아티팩트)
//   - 대신 순수 깊이 기반 호라이즌으로 elevation 계산
//   - 법선은 반구 마스킹(소프트)에만 사용 -> 부드러운 전환 보장
//
// PassCustomTable[POST_HBAO_PASS = 16]:
//   ExtValue[0] = (radius, bias, intensity, numSteps)
//   ExtValue[1] = (numDirs, texelW, texelH, falloff)

// 4×4 타일 회전 오프셋 — spatial coherence로 노이즈 최소화
// bilateral blur 가 이 16-pixel 패턴을 부드럽게 평탄화
static const float kTileAngles[16] =
{
    0.0000f, 1.5708f, 0.7854f, 2.3562f,
    2.0944f, 0.5236f, 2.8798f, 1.3090f,
    1.0472f, 2.6180f, 0.2618f, 1.8326f,
    3.1416f, 0.7854f, 2.3562f, 0.0000f,
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

float PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];

    float radius    = data.ExtValue[0].x;
    float bias      = data.ExtValue[0].y;
    float intensity = data.ExtValue[0].z;
    int   numSteps  = (int)data.ExtValue[0].w;
    int   numDirs   = (int)data.ExtValue[1].x;
    float texelW    = data.ExtValue[1].y;
    float texelH    = data.ExtValue[1].z;
    float falloff   = data.ExtValue[1].w;

    float3 posVS    = Gbuffer[1].Sample(g_sam_0, input.uv).xyz;
    float3 normalVS = normalize(Gbuffer[2].Sample(g_sam_0, input.uv).xyz);

    if (posVS.z <= 0.0f) return 1.0f;

    // ── 스크린 반경 (원근 보정 + 최대 픽셀 클램프) ──────────────
    static const float MAX_PX = 150.0f;
    float screenRadiusW = min((radius / posVS.z) / texelW, MAX_PX);
    float screenRadiusH = min((radius / posVS.z) / texelH, MAX_PX);

    // 4×4 타일 기반 방향 오프셋
    uint px = (uint)input.pos.x & 3u;
    uint py = (uint)input.pos.y & 3u;
    float randAngle = kTileAngles[py * 4u + px];

    float ao = 0.0f;

    // numDirs / numSteps 는 PassCustomTable에서 런타임에 읽는 동적 값이므로
    // [unroll] 사용 불가 — 컴파일러가 정적 루프 바운드를 알 수 없어
    // 8회 전개 후 동적 break를 마스크로 처리 → 불필요한 ALU 낭비
    // [loop] 로 변경하고 조건을 루프 헤더에 직접 기술
    [loop]
    for (int i = 0; i < numDirs; i++)
    {
        float angle = randAngle + (float)i * (PI / (float)numDirs);
        float cosA  = cos(angle);
        float sinA  = sin(angle);

        float stepW = screenRadiusW / (float)numSteps * texelW;
        float stepH = screenRadiusH / (float)numSteps * texelH;
        float2 dir2D = float2(cosA * stepW, sinA * stepH);

        float maxSinH = 0.0f;

        [loop]
        for (int j = 1; j <= numSteps; j++)
        {

            float2 sUV = input.uv + dir2D * (float)j;
            if (any(sUV < 0.0f) || any(sUV > 1.0f)) continue;

            float3 sPosVS = Gbuffer[1].Sample(g_sam_0, sUV).xyz;
            if (sPosVS.z <= 0.0f) continue;

            float3 diff = sPosVS - posVS;
            float  dist = length(diff);
            if (dist < 0.001f || dist > radius) continue;

            // ── [핵심] 깊이 기반 호라이즌 ─────────────────────────
            // 법선과 완전히 독립적 → 폴리곤 경계 아티팩트 없음
            // LH 뷰 공간: 카메라에 가까울수록(z 작을수록) "위" = 차폐 가능
            // (posVS.z - sPosVS.z) > 0 → S 가 P 보다 카메라에 가까움 → 차폐
            float sinH_depth = (posVS.z - sPosVS.z) / dist;

            // ── 반구 소프트 마스킹 (법선 사용은 여기서만) ─────────
            // dot 을 [-1,1] → [0.0, 1.0] 로 soft remapping
            // "표면 뒤" 샘플도 일부 허용 → 경계에서 부드럽게 전환
            // (하드 컷 대신 소프트 → 폴리곤 경계 아티팩트 최소화)
            float hemiMask = saturate(dot(normalVS, normalize(diff)) * 0.5f + 0.5f);

            float sinH        = sinH_depth * hemiMask;
            float attenuation = saturate(1.0f - pow(dist / radius, falloff));
            maxSinH = max(maxSinH, sinH * attenuation);
        }

        ao += saturate(maxSinH - bias);
    }

    ao = (ao / (float)numDirs) * intensity;
    return saturate(1.0f - ao);
}
