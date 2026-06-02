#include "params.hlsl"


// 공통 좌표 개념:
//  - side : 폭(가로) 방향 위치. 중심선=1, 양 끝=0. (PS_Main 에서 uv.x 로 계산)
//  - head : 길이(세로) 방향 위치. 머리(선두, 최신)=1, 꼬리=0.
//  - 형태(머리/꼬리) 판단은 흐르는 uv.y 가 아니라 ageRate 로 한다.

// 정점 셰이더 출력. 트레일 한 정점의 색/마스크 파라미터를 모두 담는다.
struct VS_OUT
{
    float4 pos : SV_Position;       // 화면 좌표
    float2 uv : TEXCOORD0;          // 트레일 UV (x=폭, y=길이, 시간에 따라 스크롤)
    float3 color : COLOR0;          // 외곽 띠 색 (EdgeColor 계열)
    float alpha : TEXCOORD1;        // 기본 알파
    float ageRate : TEXCOORD2;      // 수명 비율. 0=최신(머리), 1=소멸 직전(꼬리)
    float intensity : TEXCOORD3;    // 전체 밝기 배율 (HDR)
    float texIndex : TEXCOORD4;     // 트레일 텍스처 인덱스. 0 미만이면 텍스처 없음
    float layerIndex : TEXCOORD5;   // 다중 레이어 구분용. 노이즈 시드로 사용
    float style : TEXCOORD6;        // 트레일 종류 분기값 (PS_Main 참고)
    float cutStrength : TEXCOORD7;  // 찢어짐/녹음 강도
    float4 subColorAndLine : TEXCOORD8; // rgb=중심 코어 색, w=내부 결 세기
};

float WeaponTrailHash(float2 value)
{
    return frac(sin(dot(value, float2(127.1f, 311.7f))) * 43758.5453f);
}


float WeaponTrailNoise(float2 value)
{
    float2 cell = floor(value);                  // 격자 칸 정수 좌표
    float2 local = frac(value);                  // 칸 내부 위치 0~1
    local = local * local * (3.0f - 2.0f * local); // smoothstep 보간 가중치

    // 칸의 네 모서리 해시값
    float a = WeaponTrailHash(cell);
    float b = WeaponTrailHash(cell + float2(1.0f, 0.0f));
    float c = WeaponTrailHash(cell + float2(0.0f, 1.0f));
    float d = WeaponTrailHash(cell + float2(1.0f, 1.0f));

    // 가로로 두 번, 세로로 한 번 보간(bilinear)
    return lerp(lerp(a, b, local.x), lerp(c, d, local.x), local.y);
}

// 줄무늬(band) 생성기. value 의 소수부가 0.5 부근일 때 밝아지는 가는 띠를 만든다.
// power 가 클수록 띠가 가늘고 또렷해진다. 트레일 내부의 결(streak) 표현에 쓴다.
float WeaponTrailBand(float value, float power)
{
    float wave = abs(frac(value) * 2.0f - 1.0f); // 0~1 톱니를 삼각파로
    return pow(saturate(1.0f - wave), power);
}

// 기본 불꽃 리본 (일반적인 불타는 무기 잔상)
float4 RenderFlameRibbon(VS_OUT input, float side, float ageFade)
{
    float time = PassParams.TotalTime;
    float layer = input.layerIndex;
    // 큰 흐름 노이즈와 작은 디테일 노이즈를 섞어 불꽃 결을 만든다. time 으로 위로 흐르게 한다.
    float flowNoise = WeaponTrailNoise(float2(input.uv.y * 18.0f - time * 11.0f + layer * 2.3f, input.uv.x * 5.0f));
    float smallNoise = WeaponTrailNoise(float2(input.uv.y * 49.0f + time * 19.0f + layer * 7.1f, input.uv.x * 13.0f));
    float flameNoise = saturate(flowNoise * 0.65f + smallNoise * 0.35f);

    float edgeFade = pow(side, 0.55f);                          // 폭 전체 몸체(끝에서 0)
    float coreMask = pow(side, 2.4f) * (0.55f + flameNoise * 0.45f); // 중심에 몰린 밝은 코어
    float headBoost = lerp(0.75f, 1.25f, saturate(input.uv.y)); // 머리쪽을 더 밝게
    float head = saturate(input.uv.y);
    float tailFade = smoothstep(0.02f, 0.22f, head);            // 꼬리 부드러운 페이드아웃
    // 내부 결(가는 줄무늬 두 겹)
    float stripA = WeaponTrailBand(input.uv.x * 3.2f + input.uv.y * 1.2f + layer * 0.33f, 5.5f);
    float stripB = WeaponTrailBand(input.uv.x * 6.0f - input.uv.y * 1.7f + layer * 0.19f, 7.5f);
    float innerLines = saturate(stripA * 0.6f + stripB * 0.35f);
    // 찢어짐: 노이즈 높은 곳을 뚫어 너덜한 외곽을 만든다. 외곽(side 작음)일수록 잘 찢긴다.
    float tear = smoothstep(0.58f, 0.96f, flowNoise * 0.65f + smallNoise * 0.35f);
    float tornMask = saturate(1.0f - tear * (0.62f - side * 0.5f));

    float3 emberColor = float3(1.2f, 0.08f, 0.01f);        // 불씨(어두운 가장자리)
    float3 fireColor = input.color * 2.0f;                 // 본체 불꽃색
    float3 coreColor = float3(5.8f, 3.2f, 0.75f);          // 중심 고온 코어(HDR)
    float3 smokeHeatColor = input.subColorAndLine.rgb * 1.6f; // 외곽 연기/열기 틴트

    // 외곽 연기색에서 시작해 가장자리로 갈수록 불꽃색, 중심으로 갈수록 코어색
    float3 color = lerp(smokeHeatColor + emberColor * 0.45f, fireColor, edgeFade);
    color = lerp(color, coreColor, saturate(coreMask + innerLines * 0.32f));
    color *= input.intensity * headBoost;

    float alpha = saturate(input.alpha * ageFade * edgeFade * tailFade * tornMask);
    alpha *= lerp(0.45f, 1.25f, flameNoise); // 노이즈로 알파에 불규칙한 결을 준다
    return float4(color * alpha, alpha);      // 가산 합성을 위한 프리멀티플라이
}

// 검기
float4 RenderSwordSlash(VS_OUT input, float side, float ageFade)
{

    // - input.color          = mEdgeColor * mSlashEdgeBoost (좌우 대칭, 외곽 띠 색)
    // - input.subColorAndLine.rgb = mCoreColor * mSlashCoreBoost (중심 코어 틴트)
    // - input.subColorAndLine.w   = mSlashLineStrength
    // - input.cutStrength    = mSlashCutStrength
    // - side : 중심선=1, 폭 방향 양 끝=0
    // - uv.y : 머리(선두, 최신)=1, 꼬리=0
    float time = PassParams.TotalTime;
    float layer = input.layerIndex;
    float2 uv = input.uv;
    // 형태(머리/꼬리)는 ageRate 로 판단한다(머리=최신=ageRate≈0). uv.y 는 텍스처/결 스크롤 좌표라
    // 시간에 따라 흐르므로 형태 기준으로 쓰지 않는다.
    float head = saturate(1.0f - input.ageRate);

    // ── 폭(side) 방향 마스크 ───────────────────────────────────────────────
    // [요구4] 외곽(side→0)은 EdgeColor 가 강하게 타오르고, 중심(side→1)은 흰 코어로 밝게.
    float edgeBand = pow(saturate(1.0f - side), 2.4f);  // 양 끝에서 급격히 강해지는 외곽 띠
    float coreLine = pow(side, 3.0f);                    // 중심선에 집중되는 흰 코어
    float bodyMask = pow(side, 0.45f);                   // 폭 전체 몸체(가장자리에서 0 으로 사라짐)

    // ── 길이(uv.y) 방향 마스크 ─────────────────────────────────────────────
    // [요구5] 선두(leading, head≈1)는 하드한 모서리, 후미(trailing, head≈0)는 부드럽게 페이드.
    float trailFade = smoothstep(0.0f, 0.45f, head);                 // 꼬리쪽 넓고 부드러운 페이드아웃
    float leadingHard = smoothstep(0.80f, 0.995f, head);             // 선두로 갈수록 단단해지는 모서리
    float leadingTip = 1.0f - smoothstep(0.992f, 1.0f, head);        // 맨 앞 끝만 살짝 정리(과포화 방지)
    float lengthMask = trailFade * leadingTip;

    // ── 내부 결(streak) / 찢어짐(cut) ─────────────────────────────────────
    // [요구6] mSlashLineStrength 로 내부 결, mSlashCutStrength 로 찢어짐 강도를 제어.
    float flowA = WeaponTrailNoise(float2(uv.y * 16.0f - time * 5.0f + layer * 3.1f, uv.x * 4.0f));
    float flowB = WeaponTrailNoise(float2(uv.y * 42.0f + layer * 9.7f, uv.x * 12.0f - time * 2.0f));
    float cuts = smoothstep(0.38f, 0.92f, flowA * 0.7f + flowB * 0.3f);
    float streakA = WeaponTrailBand(uv.x * 4.7f + uv.y * 1.65f + layer * 0.21f, 6.0f);
    float streakB = WeaponTrailBand(uv.x * 7.3f - uv.y * 2.2f + layer * 0.37f, 8.0f);
    float streak = saturate(streakA * 0.8f + streakB * 0.55f);

    float lineStrength = saturate(input.subColorAndLine.w);
    // 찢어짐: 외곽(side 작음)일수록 더 잘 찢기게 하여 너덜한 베기 외곽을 만든다.
    float tornMask = lerp(1.0f, saturate(1.0f - cuts * (0.7f - side * 0.4f)), input.cutStrength);
    float lineMask = lerp(1.0f, saturate(0.55f + streak * 0.7f), lineStrength);

    // 알파
    float alpha = input.alpha * ageFade * bodyMask * lengthMask * tornMask * lineMask;
    // 선두 모서리는 하드하게 또렷이 남도록 알파를 보강.
    alpha = saturate(alpha + leadingHard * bodyMask * 0.45f);

    // erosion
    // 꼬리(head 작음)로 갈수록 절차적 노이즈로 알파를 깎아, 트레일 끝이 너덜너덜 찢기며 사라지게 한다.
    // erodeThreshold: 꼬리=1(많이 깎임), 중앙 이후=0(보존). 노이즈가 threshold 보다 큰 곳만 남긴다.
    // time 으로 노이즈를 흘려 매 프레임 다른 결로 녹아 상용 게임 같은 흩어짐을 만든다.
    // mSlashCutStrength(input.cutStrength)로 녹음 강도를 제어(0이면 녹음 없음).
    float erodeNoise = WeaponTrailNoise(float2(uv.x * 6.0f + layer * 3.7f, head * 5.0f - time * 1.5f));
    float erodeThreshold = 1.0f - smoothstep(0.0f, 0.55f, head);
    float erode = smoothstep(erodeThreshold - 0.18f, erodeThreshold + 0.18f, erodeNoise);
    alpha *= lerp(1.0f, erode, input.cutStrength);

    // 텍스처가 지정돼 있으면 절차적 마스크에 텍스처 알파를 곱
    if (input.texIndex >= 0.0f)
    {
        float4 trailTex = TextureMaps[(uint)input.texIndex].Sample(g_sam_0, frac(uv));
        alpha *= trailTex.a;
    }

    // 색
    float3 edgeColor = input.color;                 // mEdgeColor * EdgeBoost (외곽 타오르는 띠)
    float3 coreColor = input.subColorAndLine.rgb;    // mCoreColor * CoreBoost (중심 틴트)
    // 흰 코어는 흰색으로 만들어 블룸에서 뜨게 한다. 결 위에서 더 강하게.
    float3 whiteCore = float3(1.0f, 1.0f, 1.0f) * (2.0f + 4.0f * coreLine * saturate(0.5f + streak * 0.7f));

    float3 color = edgeColor * (0.5f + 1.5f * edgeBand);       // 외곽에서 EdgeColor 강조
    color = lerp(color, coreColor, coreLine * 0.7f);            // 중심으로 갈수록 코어 틴트
    color = lerp(color, whiteCore, coreLine * saturate(0.4f + streak * 0.6f)); // 중심 흰 코어
    color += edgeColor * streak * lineStrength * 0.6f;          // 내부 결 발광
    color *= input.intensity * lerp(0.85f, 1.35f, head);        // 머리쪽을 더 밝게(HDR)

    return float4(color * saturate(alpha), saturate(alpha));
}

// 망치 불꽃( 텍스처 기반의 두껍고 불씨 튀는 화염 궤적 )
float4 RenderHammerFlame(VS_OUT input, float side, float ageFade)
{
    float time = PassParams.TotalTime;
    float2 uv = input.uv;
    float layer = input.layerIndex;
    float head = saturate(uv.y);

    // 텍스처가 있으면 샘플, 없으면 기본 주황색으로 대체
    float4 trailTex = float4(1.0f, 0.55f, 0.08f, 1.0f);
    if (input.texIndex >= 0.0f)
    {
        trailTex = TextureMaps[(uint)input.texIndex].Sample(g_sam_0, saturate(uv));
    }

    // 큰 흐름 노이즈 + 미세 노이즈(불씨용)
    float flow = WeaponTrailNoise(float2(uv.y * 24.0f - time * 9.0f + layer * 2.1f, uv.x * 5.0f));
    float fine = WeaponTrailNoise(float2(uv.y * 71.0f + time * 17.0f + layer * 5.7f, uv.x * 19.0f));
    float core = pow(side, 3.2f);                       // 중심 코어
    float softBody = pow(side, 0.7f);                   // 폭 전체 몸체
    float edgeHeat = pow(saturate(1.0f - side), 2.2f);  // 양 끝 열기
    float tailIn = smoothstep(0.02f, 0.18f, head);      // 꼬리 페이드인
    float headOut = 1.0f - smoothstep(0.92f, 1.0f, head); // 머리 끝 과포화 방지
    float lengthMask = tailIn * headOut;
    float broken = lerp(0.45f, 1.15f, saturate(flow * 0.55f + fine * 0.45f)); // 노이즈로 끊긴 느낌
    // 불씨: 미세 노이즈 최상위 값만 점으로 튀게 한다. 머리/중앙폭에서 더 잘 보인다.
    float sparks = step(0.985f, fine) * smoothstep(0.35f, 1.0f, head) * smoothstep(0.15f, 0.85f, side);

    float maskAlpha = saturate(trailTex.a * broken + sparks * 0.55f);
    float alpha = saturate(input.alpha * ageFade * lengthMask * maskAlpha);
    alpha *= saturate(softBody * 0.8f + core * 0.45f + sparks * 0.5f);

    // 색을 단계별로 쌓는다: 불씨 -> 주황 본체 -> 고온 코어 -> 흰 하이라이트
    float3 ember = input.subColorAndLine.rgb * 1.4f;   // 어두운 불씨
    float3 orange = input.color * 1.9f;                // 주황 본체
    float3 hot = float3(6.0f, 3.3f, 0.65f);            // 고온 코어(HDR)
    float3 white = float3(8.0f, 5.6f, 1.4f);           // 가장 뜨거운 흰빛(HDR)
    float3 texTint = max(trailTex.rgb, float3(0.35f, 0.12f, 0.02f)); // 텍스처 색(하한 보장)
    float3 color = lerp(ember, orange, softBody);
    color = lerp(color, hot * texTint, saturate(core + trailTex.g * 0.35f));
    color = lerp(color, white, saturate(core * trailTex.a * 0.55f + sparks));
    color += input.color * edgeHeat * 0.45f;           // 양 끝 열기 가산
    color *= input.intensity * lerp(0.72f, 1.35f, head); // 머리쪽을 더 밝게

    return float4(color * alpha, alpha);
}

// 스피드라인
float4 RenderSpeedLine(VS_OUT input, float side, float ageFade)
{
    float time = PassParams.TotalTime;
    float layer = input.layerIndex;
    float head = saturate(input.uv.y);

    float core = pow(side, 4.0f);                   // 가는 중심선
    float edge = pow(side, 0.85f);                  // 폭 전체 몸체
    float tailFade = smoothstep(0.02f, 0.16f, head); // 꼬리 페이드
    // 길이 방향으로 빠르게 흐르는 노이즈와 줄무늬로 속도감을 준다.
    float flow = WeaponTrailNoise(float2(input.uv.y * 28.0f - time * 18.0f + layer * 1.7f, input.uv.x * 4.0f));
    float streak = WeaponTrailBand(input.uv.y * 9.0f - time * 5.0f + layer * 0.37f, 8.0f);
    // cutStrength 로 끊김(점선처럼 갈라지는) 강도를 제어
    float breakMask = lerp(1.0f, saturate(0.55f + flow * 0.75f + streak * 0.35f), input.cutStrength);

    float lineStrength = saturate(input.subColorAndLine.w);
    float alpha = input.alpha * ageFade * edge * tailFade * breakMask;
    alpha *= lerp(0.8f, 1.35f, streak * lineStrength); // 줄무늬 위를 더 밝게

    float3 edgeColor = input.color * 1.4f;          // 외곽 색
    float3 subColor = input.subColorAndLine.rgb * 1.2f; // 보조 색
    float3 coreColor = float3(3.8f, 5.2f, 7.0f);    // 중심 푸른 코어(HDR)
    float3 color = lerp(subColor, edgeColor, edge);
    color = lerp(color, coreColor, core);
    color *= input.intensity;

    return float4(color * saturate(alpha), saturate(alpha));
}


float4 PS_Main(VS_OUT input) : SV_Target
{
    // 폭 방향 마스크: uv.x 0~1 을 중심 1, 양 끝 0 인 삼각 모양으로 변환
    float side = 1.0f - abs(input.uv.x * 2.0f - 1.0f);
    side = saturate(side);

    // 수명 페이드: ageRate 0(최신)=1, 1(소멸)=0
    float ageFade = saturate(1.0f - input.ageRate);


    if (input.style >= 2.5f)
    {
        return RenderHammerFlame(input, side, ageFade); // 망치 불꽃
    }

    if (input.style >= 1.5f)
    {
        return RenderSpeedLine(input, side, ageFade);   // 스피드라인
    }

    if (input.style >= 0.5f)
    {
        return RenderSwordSlash(input, side, ageFade);  // 검기 베기
    }

    // 기본 불꽃 리본
    float4 color = RenderFlameRibbon(input, side, ageFade);
    if (input.texIndex >= 0.0f)
    {
        float4 trailTex = TextureMaps[(uint)input.texIndex].Sample(g_sam_0, frac(input.uv));
        color.rgb *= trailTex.rgb;
        color.a *= trailTex.a;
        color.rgb *= color.a;
    }

    return color;
}
