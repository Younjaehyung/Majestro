#ifndef _PARAMS_HLSL_
#define _PARAMS_HLSL_

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
    
    uint LightIndex; //light가 아니면 쓰지 말것.
    uint ParticleIndex; //Particle가 아니면 쓰지 말것.
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
};

struct ANIMFRAMEPARAMS
{
    float4 Scale;
    float4 Rotation; // quaternion (x,y,z,w)
    float4 Translation;
};

struct ANIMATIONMETA
{
    uint BoneCount;
    uint StartFrame;
    uint NumFrame;
    float FPS;
};


//////////////

//////////////PARTICLE
struct ComputeShared
{
    int addCount;
};

struct PARTICLESHARED
{
    int TextureIndex;
};


struct PARTICLE
{
    int Index;
    matrix MatWorld;
    
    int maxCount ;
    int addCount ;
    int frameNumber ;
    float deltaTime;
    float accTime ;
    
    float minLifeTime ;
    float maxLifeTime ;
    float minSpeed ;
    float maxSpeed ;
    float3 worldPos;
    float curTime; //경과시간
    float3 worldDir;
    float lifeTime; //유지시간
    int alive; //랜더링유무용
    float EndScale;
    float StartScale;
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
    int OcclusionMapIndex;
};
//////////////

//////////////Pass
struct PASSINFO
{
    matrix MatView;
    matrix MatProjection;
    Matrix MatViewInv; // view의 역행렬
    Matrix MatProjectionInv; // Projection의 역행렬	(사용은 선택)
    
    float2 ScreenSize;
    float2 Padding;
    
    int LightsCount;
    int SkyBoxIndex;
};
//////////////

//////////////BaseGLOBAL
struct GLOBAL_PARAMS
{

    //uint Index0;
    //uint Index1;
    //uint Index2;
    //float Index3;

    uint BaseInstanceID;
    uint etc;
};
//////////////

	
 ///////////////////////////GLOBAL_PARAMS/////////////////////////////
ConstantBuffer<GLOBAL_PARAMS> GlobalParams : register(b0, space0);
///////////////////////////////////////////////////////////////////
/*
	SHADOW, // SHADOW
	G_BUFFER, // POSITION, NORMAL, COLOR 
	LIGHTING, // DIFFUSE LIGHT, SPECULAR LIGHT*/
 ///////////////////////////G-BUFFER/////////////////////////////////
Texture2D<float4> Gbuffer[6] : register(t0, space0);
 ///////////////////////////////////////////////////////////////////


 ///////////////////////////GROUP///////////////////////////////////
ConstantBuffer<PASSINFO> PassParams : register(b0, space1);
StructuredBuffer<RENDERPARAMS> InstanceParams : register(t0, space1);
StructuredBuffer<LIGHTINFO> Lights : register(t1, space1);
StructuredBuffer<OBJECTINFO> Objects : register(t2, space1);
StructuredBuffer<PARTICLESHARED> ParticleShared : register(t3, space1); // 속성값 (SRV)
StructuredBuffer<ANIMINSTANCE> AnimInstance : register(t4, space1);
StructuredBuffer<Matrix> SFinalBone : register(t5, space1);
RWStructuredBuffer<Matrix> RFinalBone : register(u0, space1);
 ///////////////////////////////////////////////////////////////////

 ///////////////////////////PARTICLE///////////////////////////////////
StructuredBuffer<PARTICLE> Particle : register(t0, space2);     //compute Shader 결과값 읽기
RWStructuredBuffer<PARTICLE> RWParticle : register(u0,space2); //compute Shader 결과값 쓰기
RWStructuredBuffer<ComputeShared> RWParticleShared : register(u1,space2); //공유 전역변수
 ///////////////////////////////////////////////////////////////////

 ///////////////////////////ANIMATION///////////////////////////////
StructuredBuffer<matrix> SkeletonBone : register(t0, space3);
StructuredBuffer<ANIMFRAMEPARAMS> AnimationClip : register(t1, space3);
StructuredBuffer<ANIMATIONMETA> AnimationMeta : register(t2, space3);
 ///////////////////////////////////////////////////////////////////

 ////////////////////////////TEXTURE////////////////////////////////
StructuredBuffer<MATERIALINFO> Materials : register(t0, space4);
TextureCube SkyBoxMaps[16] : register(t1, space4);
Texture2D<float4> TextureMaps[1024] : register(t17, space4);
 ///////////////////////////////////////////////////////////////////




SamplerState g_sam_0 : register(s0);

#endif
