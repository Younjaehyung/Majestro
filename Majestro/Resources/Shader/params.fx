#ifndef _PARAMS_FX_
#define _PARAMS_FX_

struct LightColor
{
    float4      diffuse;
    float4      ambient;
    float4      specular;
};

struct LightInfo
{
    LightColor  color;
    float4	    position;
    float4	    direction; 
    int		    lightType;
    float	    range;
    float	    angle;
    int  	    padding;
};

struct TRANSFORM
{
        //row_major: 행렬접근순서를 다렉기준으로 정의함
    row_major matrix MatWorld;
    row_major matrix MatView;
    row_major matrix g_matProjection;

    
    row_major matrix g_matViewInv;
};

struct PARTICLE
{
    int Index;
    row_major matrix MatWorld;
    
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
    float3 padding;
};

struct MATERIAL
{
    int g_int_0;
    int g_int_1;
    int g_int_2;
    int g_int_3;

    float g_float_0;
    float g_float_1;
    float g_float_2;
    float g_float_3;

    
    //texture 유무 확인용
    int g_tex_on_0;
    int g_tex_on_1;
    int g_tex_on_2;
    int g_tex_on_3;

    
    //
    float2 g_vec2_0;
    float2 g_vec2_1;
    float2 g_vec2_2;
    float2 g_vec2_3;
    
    float4 g_vec4_0;
    float4 g_vec4_1;
    float4 g_vec4_2;
    float4 g_vec4_3;
    
    row_major float4x4 g_mat_0;
    row_major float4x4 g_mat_1;
    row_major float4x4 g_mat_2;
    row_major float4x4 g_mat_3;
};

struct ETC
{
    int SkyBoxIndex;
};

struct GLOBAL_PARAMS
{
    int MaterialsIndexStart;
    int MaterialsIndexSize;
    int TransformIndex;
};

struct CAMERA_PARAMS
{
    Matrix MatView;
    Matrix MatProjection;
    Matrix MatViewInv; // view의 역행렬
    Matrix MatProjectionInv; // Projection의 역행렬	(사용은 선택)
};


Texture2D<float4> Gbuffer[6] : register(t0, space0);

ConstantBuffer<GLOBAL_PARAMS> GlobalParams : register(b0, space1);
ConstantBuffer<CAMERA_PARAMS> CameraParams : register(b1, space1);
ConstantBuffer<ETC> ETCParams : register(b5, space1);

StructuredBuffer<LightInfo> Lights : register(t0,space1);
StructuredBuffer<TRANSFORM> Transforms : register(t1, space1);
StructuredBuffer<MATERIAL> Materials : register(t2, space1);
StructuredBuffer<PARTICLE> Particle : register(t3, space1);
//StructuredBuffer<Matrix> g_mat_bone : register(t4);
 

Texture2D<float4> TextureMaps[3] : register(t0, space2);
TextureCube SkyBoxMaps[16] : register(t1, space2);


SamplerState g_sam_0 : register(s0);

#endif