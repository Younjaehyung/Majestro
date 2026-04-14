#include "params.hlsl"

// 깊이(뷰 공간 Z) 차이를 엣지 기준으로 사용하는 분리 가능 블러.
// DX12 LH 뷰 공간: 유효 표면 픽셀 = z > 0 (스카이박스 = z <= 0)
//
// PassCustomTable[POST_HBAO_BLUR_PASS = 17]:
//  etc=0 : 가로 ExtValue[0] = (texelW, texelH, float(rawAoIdx),  blurRadius)  
//  etc=1 : 세로 ExtValue[1] = (texelW, texelH, float(blurRtIdx), blurRadius)  
//
// GlobalParams.etc = 0(가로) / 1(세로)
// 깊이 엣지: Gbuffer[1].z (뷰 공간 깊이, 양수 = 유효)

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD;
};

static const float SIGMA_Z   = 0.3f;  // 깊이 허용 범위 (뷰 공간 단위)
static const int   MAX_RADIUS = 5;    // 최대 tap 반경

float GaussianWeight(float x, float sigma)
{
    return exp(-0.5f * (x * x) / (sigma * sigma));
}

float PS_Main(VS_OUT input) : SV_Target
{
    PASS_CUSTOM_DATA data = PassCustomTable[GlobalParams.PassCustomIndex];
    uint             dir  = GlobalParams.etc; // 0=가로, 1=세로

    float  texelW     = data.ExtValue[dir].x;
    float  texelH     = data.ExtValue[dir].y;
    uint   srcIdx     = (uint)data.ExtValue[dir].z;
    float  blurRadius = data.ExtValue[dir].w;

    // 블러 방향 벡터 (텍셀 단위)
    float2 blurDir = (dir == 0)
        ? float2(texelW, 0.0f)
        : float2(0.0f, texelH);

    // 중심 픽셀
    float centerAo    = TextureMaps[srcIdx].Sample(g_sam_0, input.uv).r;
    float centerDepth = Gbuffer[1].Sample(g_sam_0, input.uv).z; // 뷰 공간 Z (LH: 양수=유효)

    // 스카이박스: 블러 없이 통과 (z <= 0 = 배경)
    if (centerDepth <= 0.0f)
        return 1.0f;

    float weightedSum = centerAo * 1.0f; // 중심 픽셀 가중치=1
    float totalWeight = 1.0f;

    int tapCount = clamp((int)blurRadius, 1, MAX_RADIUS);

    [loop]
    for (int i = -tapCount; i <= tapCount; i++)
    {
        if (i == 0) continue; // 중심은 위에서 처리

        float2 sUV    = saturate(input.uv + blurDir * (float)i);
        float  sAo    = TextureMaps[srcIdx].Sample(g_sam_0, sUV).r;
        float  sDepth = Gbuffer[1].Sample(g_sam_0, sUV).z;

    
        if (sDepth <= 0.0f) continue;

        // 공간 가우시안 (거리에 따른 가중치)
        float wSpatial = GaussianWeight((float)abs(i), blurRadius * 0.5f);

        // 깊이 차이 기반 엣지 보존 가중치
        float depthDiff = abs(sDepth - centerDepth);
        // 깊이 차이가 클수록(엣지) 가중치 0에 가까워짐
        // centerDepth 로 정규화: 절대 단위 대신 상대 비율 사용
        float wDepth = GaussianWeight(depthDiff / (centerDepth * 0.01f + 0.1f), SIGMA_Z);

        float w = wSpatial * wDepth;
        weightedSum += sAo * w;
        totalWeight += w;
    }

    return weightedSum / max(totalWeight, 1e-5f);
}
