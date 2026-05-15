#ifndef _PARAMS_HLSL_
#define _PARAMS_HLSL_


#define RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT 3

//////////////Light
struct LightColor
{
    float4      diffuse;
    float4      ambient;
    float4      specular;
};

struct LIGHTINFO
{
    LightColor  color;
    float4	    position;
    float4	    direction; 
    int		    lightType;
    float	    range;
    float	    angle;
    int         padding; // 데이터 사이즈용 padding
    
    matrix MatWorld;
    matrix MatView;
    matrix MatProjection;
    matrix MatViewInv;
    matrix MatProjectionInv;
    

};
//////////////

//////////////Object
struct OBJECTINFO
{
    matrix MatWorld;
    // x = ObjectAlpha
    // y/z/w = 예비
    float4 Extra;
};
//////////////

//////////////Instance
struct RENDERPARAMS
{

    //uint Index0;
    //uint Index1;
    //uint Index2;
    //uint Index3;
    
    
    uint ObjectIndex;
    uint MaterialInfoIndex;
    
    int LightIndex; //light가 아니면 쓰지 말것
    uint ParticleIndex; //Particle이 아니면 쓰지 말것
};
//////////////


//////////////Animation
struct SkinningInfo
{
    float3 pos;
    float3 normal;
    float3 tangent;
};

struct ANIMINSTANCE
{
    uint SkeletonID;
    uint AnimClipIdx;
    uint CurrentFrame;
    uint NextFrame;
    float Ratio;
    
    uint BoneCount;
    uint ReulstIndex;
    uint EntityID;
    uint BlendClipIdx;
    uint BlendCurrentFrame;
    uint BlendNextFrame;

    float BlendRatio;

    float BlendWeight;
    uint BlendMaskStart;
    uint BlendMaskEnd;
    uint BlendMode;

    uint UpperAnimClipIdx;
    uint UpperCurrentFrame;
    uint UpperNextFrame;
    float UpperRatio;

    uint UpperBlendClipIdx;
    uint UpperBlendCurrentFrame;
    uint UpperBlendNextFrame;
    float UpperBlendRatio;

    float UpperBlendWeight;
    float UpperLayerWeight;
    uint UpperMaskStart;
    uint UpperMaskEnd;
    uint UpperBlendMode;
};

struct ANIMFRAMEPARAMS
{
    float4 Scale;
    float4 Rotation; // quaternion (x,y,z,w)
    float4 Translation;
};

struct ANIMATIONMETA
{
    uint BoneStart;
    uint BoneCount;
    uint StartFrame;
    uint NumFrame;
    uint AnimOffset;
};

struct SKELETONBONE
{
    matrix Offset;
    float BlendWeight;
    int ParentIdx;
    float2 Padding;
};

//////////////

//////////////PARTICLE
struct ComputeShared
{
    int addCount;
    int3 padding;
};

struct PARTICLESHARED
{
    matrix MatWorld;
    int TextureIndex;
    int maxCount;
    int addCount;
    float deltaTime;
    float accTime;
    float minLifeTime;
    float maxLifeTime;
    float minSpeed;
    float maxSpeed;
    float StartScale;
    float EndScale;
};


struct PARTICLE
{
    float3 worldPos;
    float curTime;
    float3 worldDir;
    float lifeTime;
    int alive;
    float3 padding;
};
//////////////

//////////////MATERIALI
struct MATERIALINFO
{
    float4 Diffuse;
    float4 Ambient;
    float4 Specular;
    float3 Emission;
    float Metallic;
    float Roughness;
    
    uint OcclusionMask;
    uint AlphaTest;
    
    int DiffuseMap0Index;
    int DiffuseMap1Index;
    int DiffuseMap2Index;
    int DiffuseMap3Index;
    
    int NormalMapIndex;
    int SpecularcMapIndex;
    int EmissiveMapIndex;
    int MetallicMapIndex;
    int RoughnessMapIndex;
    int OcclusionMapIndex;
    
    int MaskMapIndex;

    int ExtTex[8]; // 머티리얼별 자유 텍스처 슬롯 8개 (-1 = 미사용)
    float4 ExtValue[4]; // 머티리얼별 자유 파라미터 슬롯 4개
};
//////////////

//////////////UI
struct UIInstanceData
{
    float2 Position; // 픽셀 좌표 (좌상단 기준)
    float2 Size; // 픽셀 크기
    
    float2 Pivot; // (0~1)
    uint   MaterialIndex;
    float ZOrder; // 정렬용
};
//////////////



//////////////Pass
struct PASSINFO
{
    matrix MatPrevView;
    matrix MatPrevProjection;
    Matrix MatPrevViewInv; // view의 역행렬
    Matrix MatPrevProjectionInv; // Projection의 역행렬    (사용은 선택)
    
    matrix MatView;
    matrix MatProjection;
    Matrix MatViewInv; // view의 역행렬
    Matrix MatProjectionInv; // Projection의 역행렬    (사용은 선택)
    
    float2 ScreenSize;
    float2 MinMaxTessDistance;

    float2 HeightMapResolution;
    float MaxTessLevel;
    float TotalTime;
    
    int TileCountX;
    int TileCountZ;
    int LightsCount;
    int SkyBoxIndex;
    
    int TerrainSlot1;
    int TerrainSlot2;
    int TerrainSlot3;
    int TerrainSlot4;
    
    int TerrainSlot5;
    int TerrainSlot6;
    int IrradianceIndex; // CubeBoxMaps 배열 인덱스 (IBL Diffuse)
    int PreFilteredEnvIndex; // CubeBoxMaps 배열 인덱스 (IBL Specular)

	float4 CascadeSplitDistances;

    matrix CascadeShadowVP[RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT];

    // IBL 추가 파라미터
    int BrdfLutIndex; // TextureMaps 배열 인덱스 (BRDF LUT 2D)
    int PreFilteredMipLevels; // pre-filtered 큐브맵 mip 단계 수
    int IBLPadding0;
    int IBLPadding1;
    
    //int PassTexture1;
    //int PassTexture2;
    //int PassTexture3;
    //int PassTexture4;
    
    //int PassTexture5;
    //int PassTexture6;
    //int PassTexture7;
    //int PassTexture8;
    
    //float4 PassValue1;
    //float4 PassValue2;
    //float4 PassValue3;
    //float4 PassValue4;
    
    //float4 PassValue5;
    //float4 PassValue6;
    //float4 PassValue7;
    //float4 PassValue8;
};
//////////////

//////////////BaseGLOBAL
struct GLOBAL_PARAMS
{
    uint BaseInstanceID;
    uint etc; // WorldUIPass 한정 플래그 — bit0: 1=HUD(screen-space, no depth occlusion), 0=World
    uint casdcae;
    uint PassCustomIndex; // PASS_CUSTOM_DATA 테이블 행 인덱스 (PASS_CUSTOM_INDEX 열거형)

    //HP 바 (WorldUIPass) per-bar 데이터
    float3 HpBarAnchorWorld;  // 월드 공간 앵커 위치
    float  HpBarFollowRatio;  // 현재 HP 비율 (0~1) — fill 크롭용 (r1.w 빈 lane 활용)

    float2 HpBarSizePx;       // 화면 픽셀 단위 바 크기
    float2 HpBarPivotPx;      // 앵커 기준 바 좌상단 픽셀 오프셋 (보통 -size/2)

    uint   HpBarBgTexIdx;     // TextureMaps 배열 인덱스 (배경)
    uint   HpBarFillTexIdx;   // TextureMaps 배열 인덱스 (채움)
    uint   HpBarHitTexIdx;    // TextureMaps 배열 인덱스 (hit effect 시트, 0이면 비활성)
    uint   HpBarHitConfig;    // packed: cols (bits 0-7) | rows (bits 8-15) | frameCount (bits 16-31)
};
//////////////

//////////////PassCustom
struct PASS_CUSTOM_DATA
{
    int ExtTex[8]; // 자유 텍스처 슬롯 (TextureMaps 배열 인덱스, -1 = 미사용)
    float4 ExtValue[4]; // 자유 파라미터 슬롯 (float4 × 4)
    
    int PreviousStep;
};
//////////////

	
 ///////////////////////////GLOBAL_PARAMS/////////////////////////////
ConstantBuffer<GLOBAL_PARAMS> GlobalParams : register(b0, space0);
///////////////////////////////////////////////////////////////////
/*
	    SHADOW, // SHADOW [ 기존 0] 
	G_BUFFER, // POSITION, NORMAL, COLOR 
	LIGHTING, // DIFFUSE LIGHT, SPECULAR LIGHT*/
 ///////////////////////////G-BUFFER/////////////////////////////////
// Gbuffer
// [0]=PreDepth [1]=Position [2]=Normal [3]=Albedo
// [4]=Emissive [5]=DiffuseLight [6]=SpecularLight [7]=HDR
// [8]=PostHDR_A [9]=PostHDR_B [10]=PostLDR_A [11]=PostLDR_B [12]=MotionVec [13]=Cascade
Texture2D<float4> Gbuffer[13] : register(t0, space0);
Texture2DArray ShadowMaps : register(t13, space0);
 ///////////////////////////////////////////////////////////////////


 ///////////////////////////GROUP///////////////////////////////////
ConstantBuffer<PASSINFO> PassParams : register(b0, space1);
StructuredBuffer<RENDERPARAMS> InstanceParams : register(t0, space1);
StructuredBuffer<LIGHTINFO> Lights : register(t1, space1);
StructuredBuffer<OBJECTINFO> Objects : register(t2, space1);
StructuredBuffer<PARTICLESHARED> ParticleShared : register(t3, space1); // 속성값 (SRV)
StructuredBuffer<UIInstanceData> UIInstances : register(t4, space1);
StructuredBuffer<ANIMINSTANCE> AnimInstance : register(t5, space1);
StructuredBuffer<Matrix> SFinalBone : register(t6, space1);
StructuredBuffer<uint2> ForwardPlusTileMeta : register(t7, space1);
StructuredBuffer<uint> ForwardPlusLightIndices : register(t8, space1);
StructuredBuffer<PASS_CUSTOM_DATA> PassCustomTable : register(t9, space1); // pass별 커스텀 텍스처/파라미터 테이블
RWStructuredBuffer<Matrix> RFinalBone : register(u0, space1);
RWStructuredBuffer<uint2> RWForwardPlusTileMeta : register(u1, space1);
RWStructuredBuffer<uint> RWForwardPlusLightIndices : register(u2, space1);

 ///////////////////////////////////////////////////////////////////

 ///////////////////////////PARTICLE///////////////////////////////////
StructuredBuffer<PARTICLE> Particle : register(t0, space2);     //compute Shader 결과값 읽기
RWStructuredBuffer<PARTICLE> RWParticle : register(u0,space2); //compute Shader 결과값 쓰기
RWStructuredBuffer<ComputeShared> RWParticleShared : register(u1,space2); // frame-shared spawn counters
 ///////////////////////////////////////////////////////////////////

 ///////////////////////////ANIMATION///////////////////////////////
StructuredBuffer<SKELETONBONE> SkeletonBone : register(t0, space3);
StructuredBuffer<ANIMFRAMEPARAMS> AnimationClip : register(t1, space3);
StructuredBuffer<ANIMATIONMETA> AnimationMeta : register(t2, space3);
 ///////////////////////////////////////////////////////////////////

 ////////////////////////////TEXTURE////////////////////////////////
StructuredBuffer<MATERIALINFO> Materials : register(t0, space4);
TextureCube CubeBoxMaps[15] : register(t1, space4);
Texture2D<float4> TextureMaps[2048] : register(t16, space4);
 ///////////////////////////////////////////////////////////////////




SamplerState g_sam_0 : register(s0);
SamplerState g_sam_Terrain : register(s1);
SamplerComparisonState g_sam_shadow : register(s2); // 그림자 PCF 비교 샘플러

#endif 

