#include "params.hlsl"
#include "math.hlsl"

// MATERIALINFO.ExtValue 파라미터 레이아웃
//   ExtValue[0].z = scrollSpeed      UV 스크롤 속도
//   ExtValue[0].w = flowStrength     Flow Map 왜곡 세기
//
//   ExtValue[1].xyz = shallowColor   시선이 스칠 때 틴트 색 (RGB)
//   ExtValue[2].xyz = deepColor      물 본체 색 (RGB, 정면에서 보이는 색)
//
//   ExtValue[3].x = texTiling          UV 타일링 배율
//   ExtValue[3].z = foamBrightness     거품 밝기
//   ExtValue[3].w = surfaceFoamAmount  표면 whitecap 거품 강도 (0=비활성)
//   (ExtValue[3].y depthFadeScale 는 더 이상 사용 안 함)
//
//   MATERIALINFO.ExtTex
//   ExtTex[1] = Flow Map 텍스처    (-1이면 단순 UV)
//   ExtTex[2] = 물 노멀맵 (-1이면 노멀맵 찰랑임/스펙큘러 비활성 + Voronoi 거품 폴백)
//   (ExtTex[0] 해안선 거품 텍스처는 더 이상 사용 안 함)


struct VS_OUT
{
    float4 pos         : SV_Position;
    float2 uv          : TEXCOORD;
    float3 viewPos     : POSITION;
    float3 viewNormal  : NORMAL;
    float3 viewTangent : TANGENT;
    uint   instanceID  : InstanceID;
};

// 노멀맵 찰랑임 튜닝 상수 (ExtTex[2] = 물 노멀맵일 때만 동작
static const float OCEAN_NORMAL_STRENGTH  = 0.35f;  // 표면 노멀 왜곡 세기 (0=평평)
static const float OCEAN_NORMAL_TILING    = 0.15f;  // 노멀맵 타일링 (baseUV에 곱; baseUV가 이미 texTiling 적용됨)
static const float OCEAN_SPEC_POWER       = 160.0f; // 스펙큘러 광택 (클수록 또렷한 점)
static const float OCEAN_SPEC_INTENSITY   = 0.8f;   // 스펙큘러 세기
static const float OCEAN_FRESNEL_STRENGTH = 0.25f;  // 프레넬 가장자리 밝기 (원경 반짝임)
// whitecap 거품 ↔ 노멀맵 결합 튜닝 (ExtTex[2] = 물 노멀맵일 때만 동작)
static const float OCEAN_FOAM_SLOPE_LO    = 0.25f;  // 거품 시작 슬로프 (작을수록 거품이 넓게 퍼짐)
static const float OCEAN_FOAM_SLOPE_HI    = 0.55f;  // 거품 포화 슬로프 (마루 정점만 흰 거품)

// 2D 해시 — 절차적 Voronoi/거품용
float2 Hash22(float2 p)
{
    p = float2(dot(p, float2(127.1f, 311.7f)),
               dot(p, float2(269.5f, 183.3f)));
    return frac(sin(p) * 43758.5453123f);
}

// Voronoi
float Voronoi2D(float2 uv)
{
    float2 g = floor(uv);
    float2 f = frac(uv);
    float  res = 8.0f;
    [unroll] for (int j = -1; j <= 1; ++j)
    [unroll] for (int i = -1; i <= 1; ++i)
    {
        float2 lattice = float2(i, j);
        float2 offset  = Hash22(g + lattice);
        float2 r       = lattice + offset - f;
        res = min(res, dot(r, r));
    }
    return sqrt(res);
}

float4 PS_Main(VS_OUT input) : SV_Target
{
    const uint         idx  = GlobalParams.BaseInstanceID + input.instanceID;
    const RENDERPARAMS inst = InstanceParams[idx];
    const MATERIALINFO mtl  = Materials[inst.MaterialInfoIndex];

    float scrollSpeed     = mtl.ExtValue[0].z;
    float flowStrength    = mtl.ExtValue[0].w;

    float3 shallowColor   = mtl.ExtValue[1].xyz; // 시선 스칠 때 틴트
    float3 deepColor      = mtl.ExtValue[2].xyz; // 물 본체 색

    float  texTiling         = max(mtl.ExtValue[3].x, 0.001f);
    float  foamBrightness    = mtl.ExtValue[3].z;
    float  surfaceFoamAmount = mtl.ExtValue[3].w; // 표면 whitecap 거품 강도 (0이면 비활성)

    float t = PassParams.TotalTime * scrollSpeed;

    // UV 설정
    float2 baseUV = input.uv * texTiling;

    if (mtl.ExtTex[1] >= 0)
    {
        // Flow Map
        float2 flowDir = TextureMaps[mtl.ExtTex[1]].Sample(g_sam_0, baseUV).rg * 2.0f - 1.0f;
        float  phase1  = frac(t);
        float  phase2  = frac(t + 0.5f);
        float  blend   = abs(frac(t) * 2.0f - 1.0f);
        float2 uv1     = baseUV + flowDir * (flowStrength * phase1);
        float2 uv2     = baseUV + flowDir * (flowStrength * phase2);
        baseUV = lerp(uv1, uv2, blend);
    }

    float3 specColor    = float3(0.0f, 0.0f, 0.0f);
    float  fresnel      = 0.0f;
    float  surfaceSlope = 0.0f; 
    if (mtl.ExtTex[2] >= 0)
    {
        float2 nuvA = baseUV * OCEAN_NORMAL_TILING        + float2( t * 0.9f,  t * 0.6f);
        float2 nuvB = baseUV * OCEAN_NORMAL_TILING * 1.7f + float2(-t * 0.7f,  t * 0.9f);
        float3 nA = TextureMaps[mtl.ExtTex[2]].Sample(g_sam_0, nuvA).xyz * 2.0f - 1.0f;
        float3 nB = TextureMaps[mtl.ExtTex[2]].Sample(g_sam_0, nuvB).xyz * 2.0f - 1.0f;

        float2 nSum  = (nA.xy + nB.xy) * OCEAN_NORMAL_STRENGTH;
        surfaceSlope = length(nSum);                 
        float3 tn    = normalize(float3(nSum, 1.0f));

        // 뷰 공간 TBN
        float3 N0 = normalize(input.viewNormal);
        float3 T  = normalize(input.viewTangent - N0 * dot(input.viewTangent, N0));
        float3 B  = cross(N0, T);
        float3 N  = normalize(T * tn.x + B * tn.y + N0 * tn.z);

        float3 V = normalize(-input.viewPos); 

        // 방향광(태양)
        float3 sunDiffuse = float3(1.0f, 1.0f, 1.0f);
        float3 L = N;
        float3 viewLightDir = normalize(
            mul(float4(PassParams.DirectionalLight.direction.xyz, 0.f),
                PassParams.MatView).xyz);
        L = normalize(-viewLightDir);
        sunDiffuse = PassParams.DirectionalLight.color.diffuse.rgb;

        // 햇빛 반짝임 (Blinn-Phong 스펙큘러)
        float3 H    = normalize(L + V);
        float  spec = pow(saturate(dot(N, H)), OCEAN_SPEC_POWER);
        specColor   = sunDiffuse * (spec * OCEAN_SPEC_INTENSITY);

        // 프레넬
        fresnel = OCEAN_FRESNEL_STRENGTH * pow(1.0f - saturate(dot(N, V)), 5.0f);
    }

    // 표면 whitecap
    float whitecap;
    if (mtl.ExtTex[2] >= 0)
    {
        whitecap = smoothstep(OCEAN_FOAM_SLOPE_LO, OCEAN_FOAM_SLOPE_HI, surfaceSlope) * surfaceFoamAmount;
    }
    else
    {
        // 절차적 Voronoi
        float v1   = Voronoi2D(baseUV * 0.6f + float2( t * 1.4f,  t * 0.8f));
        float v2   = Voronoi2D(baseUV * 1.1f + float2(-t * 0.9f,  t * 1.2f));
        float edge = saturate(v1 * 0.6f + v2 * 0.4f);
        whitecap   = smoothstep(0.45f, 0.80f, edge) * surfaceFoamAmount;
    }

    // 최종 거품
    float foam = saturate(whitecap);

    // 색상 합성
    float3 waterColor = lerp(deepColor, shallowColor, saturate(fresnel * 4.0f));
    float3 foamColor  = float3(foamBrightness, foamBrightness, foamBrightness);
    float3 finalColor = lerp(waterColor, foamColor, foam);
    finalColor += specColor + fresnel;          // 노멀맵 찰랑임 반짝임 가산

    return float4(finalColor, 1.0f);            // 불투명
}
