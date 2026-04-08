#include "params.hlsl"
#include "utils.hlsl"


// PASS_CUSTOM_DATA 레이아웃-------------------------------------------------------------
//  PreviousStep     : HDR RT (이전 패스 출력) 인덱스
//  ExtValue[0].xyz  : sunDir (World Space, 태양으로부터 오는 방향 — 빛 방향의 반대)
//  ExtValue[0].w    : intensity
//  ExtValue[1].x    : numSteps (float에서 int 캐스트)
//  ExtValue[1].y    : maxRayLen (월드 유닛, 하늘 픽셀 레이 최대 길이)
//  ExtValue[1].z    : scatterCoeff (산란 계수, 매질 밀도)
//  ExtValue[1].w    : mieAsymmetry  (g, Henyey-Greenstein -1~1)
//  ExtValue[2].xyz  : sunColor (HDR 선형 컬러)
//  ExtValue[2].w    : absorptionCoeff (흡수 계수)
// -------------------------------------------------------------

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};


float4 PS_Main(VS_OUT input) : SV_Target
{

    uint passIdx         = GlobalParams.PassCustomIndex;
    PASS_CUSTOM_DATA data = PassCustomTable[passIdx];

    float4 sceneColor = Gbuffer[data.PreviousStep].Sample(g_sam_0, input.uv);

    float3 sunDir         = normalize(data.ExtValue[0].xyz);   // 태양으로부터 오는 방향 (World Space)
    float  intensity      = data.ExtValue[0].w;
    int    numSteps       = max((int)data.ExtValue[1].x, 1);
    float  maxRayLen      = max(data.ExtValue[1].y, 1.0f);
    float  scatterCoeff   = data.ExtValue[1].z;
    float  mieG           = data.ExtValue[1].w;
    float3 sunColor       = data.ExtValue[2].xyz;
    float  absorptionCoeff = data.ExtValue[2].w;

    // Depth에서 View Position 복원 
    float depth = Gbuffer[0].Sample(g_sam_0, input.uv).r;

    float2 ndc;
    ndc.x =  input.uv.x * 2.0f - 1.0f;
    ndc.y = -input.uv.y * 2.0f + 1.0f;

    float4 clipPos  = float4(ndc, depth, 1.0f);
    float4 viewPos4 = mul(clipPos, PassParams.MatProjectionInv);
    viewPos4 /= viewPos4.w;

    float3 viewPos = viewPos4.xyz;

    // World Position & 레이 정의 
    float4 worldPos4 = mul(float4(viewPos, 1.0f), PassParams.MatViewInv);
    float3 worldPos  = worldPos4.xyz;

    // 카메라 위치 (View 역행렬 Translation 컬럼)
    float3 camPos = float3(
        PassParams.MatViewInv._41,
        PassParams.MatViewInv._42,
        PassParams.MatViewInv._43);

    float3 rayVec = worldPos - camPos;
    float  rayLen = length(rayVec);
    float3 rayDir = normalize(rayVec);

    // 하늘 픽셀(depth = 1)은 maxRayLen으로 클리핑
    bool  isSky       = (depth >= 0.9999f);
    float actualRayLen = isSky ? maxRayLen : min(rayLen, maxRayLen);

    // Mie 위상 함수
    float cosTheta = dot(rayDir, -sunDir); // sunDir은 카메라 방향 기준이므로 반전
    float phase    = MiePhase(cosTheta, mieG);

    //  레이마칭
    float dstep = actualRayLen / (float)numSteps;

    // Bayer 지터로 시작 오프셋 -> 밴딩 아티팩트 억제
    float jitter       = BayerDither(input.pos.xy) * dstep;
    float3 accumulated = 0.0f;
    float  transmittance = 1.0f;

    float3 currentWorldPos = camPos + rayDir * jitter;

    [loop]
    for (int i = 0; i < numSteps; ++i)
    {
        currentWorldPos += rayDir * dstep;

        // World -> View (CalculateCSMShadow는 View Space 입력)
        float4 curView4 = mul(float4(currentWorldPos, 1.0f), PassParams.MatView);
        float3 curView  = curView4.xyz;

        // Cascade Shadow Map으로 빛 가시성 계산
        // viewNormal은 VLS에서 임의값 사용 (bias 계산에만 관여, 법선 없음)
        float visibility = CalculateCSMShadow(curView, float3(0.0f, 1.0f, 0.0f), sunDir);

        // In-scattering 누적 (Beer-Lambert 감쇠 가중)
        float3 Li = sunColor * phase * scatterCoeff * visibility;
        accumulated += Li * transmittance * dstep;

        // Beer-Lambert 감쇠
        transmittance *= exp(-(scatterCoeff + absorptionCoeff) * dstep);


        if (transmittance < 0.005f)
            break;
    }

    accumulated *= intensity;

    return float4(sceneColor.rgb + accumulated, sceneColor.a);
}
