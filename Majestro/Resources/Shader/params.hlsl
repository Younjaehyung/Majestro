#ifndef _PARAMS_HLSL_
#define _PARAMS_HLSL_
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
    int  	    padding;
    
    
    matrix MatWorld;
    matrix MatView;
    matrix MatProjection;
    matrix MatViewInv;
    matrix MatProjectionInv;
    
    int MaterialsIndex;
};

struct OBJECTINFO
{
    matrix MatWorld;
    
    int MaterialInfoIndex;
    
    int IsParticleSystem; // Object가 아니라 파티클이면 1
};

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

struct MATERIALINFO
{
    float4 Diffuse;
    
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
    int EmissiveMapIndex;
    int MetallicMapIndex;
    int OcclusionMapIndex;
};

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

struct GLOBAL_PARAMS
{

    uint ObjectIndex;
    uint LightIndex;    //light가 아니면 쓰지 말것.
    uint ParticleIndex; //Particle가 아니면 쓰지 말것.

};
/*
	
	SHADOW, // SHADOW
	G_BUFFER, // POSITION, NORMAL, COLOR 
	LIGHTING, // DIFFUSE LIGHT, SPECULAR LIGHT*/
 ///////////////////////////G-BUFFER/////////////////////////////////
Texture2D<float4> Gbuffer[6] : register(t0, space0);
 ///////////////////////////////////////////////////////////////////


 ///////////////////////////GROUP///////////////////////////////////
ConstantBuffer<GLOBAL_PARAMS> GlobalParams : register(b0, space1);
ConstantBuffer<PASSINFO> PassParams : register(b1, space1);

StructuredBuffer<LIGHTINFO> Lights : register(t0, space1);
StructuredBuffer<OBJECTINFO> Objects : register(t1, space1);
StructuredBuffer<MATERIALINFO> Materials : register(t2, space1);


//StructuredBuffer<Matrix> g_mat_bone : register(t4);
 ///////////////////////////////////////////////////////////////////
StructuredBuffer<PARTICLE> Particle : register(t3, space1);
StructuredBuffer<PARTICLESHARED> ParticleShared : register(t4, space1);


 ////////////////////////////////UAV////////////////////////////////
RWStructuredBuffer<PARTICLE> RWParticle : register(u0,space0); //compute Shader 결과값 저장
RWStructuredBuffer<ComputeShared> RWParticleShared : register(u1,space0); //공유 전역변수
 ///////////////////////////////////////////////////////////////////

 ////////////////////////////TEXTURE////////////////////////////////
Texture2D<float4> TextureMaps[1000] : register(t0, space2);
TextureCube SkyBoxMaps[16] : register(t1, space2);
 ///////////////////////////////////////////////////////////////////



SamplerState g_sam_0 : register(s0);

#endif
