#ifndef _PARTICLE_FX_
#define _PARTICLE_FX_

#include "params.hlsli"
#include "utils.fx"



struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    uint id : SV_InstanceID;    //instance ID
};

struct VS_OUT
{
    float4 viewPos : POSITION;
    float2 uv : TEXCOORD;
    float id : ID;
};

// VS_MAIN
// g_float_0    : Start Scale
// g_float_1    : End Scale
// g_tex_0      : Particle Texture

VS_OUT VS_Main(VS_IN input)
{
    VS_OUT output = (VS_OUT) 0.f;

    float3 worldPos = mul(float4(input.pos, 1.f), Objects[GlobalParams.ObjectIndex].MatWorld).xyz;
    worldPos += RWParticle[input.id].worldPos;

    output.viewPos = mul(float4(worldPos, 1.f), PassParams.MatView);
    output.uv = input.uv;
    output.id = input.id;

    return output;
}

struct GS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    uint id : SV_InstanceID;
};

[maxvertexcount(6)] //vertex Shdaer에서 점 하나만 옮겨 확인하기로 했기에 point임
void GS_Main(point VS_OUT input[1], inout TriangleStream<GS_OUT> outputStream)
{
    GS_OUT output[4] =
    {
        (GS_OUT) 0.f, (GS_OUT) 0.f, (GS_OUT) 0.f, (GS_OUT) 0.f
    };

    VS_OUT vtx = input[0];
    uint id = (uint) vtx.id;
    if (0 == Particle[id].alive)
        return;

    float ratio = Particle[id].curTime / Particle[id].lifeTime;
    float scale = ((Particle[id].EndScale - Particle[id].StartScale) * ratio + Particle[id].StartScale) / 2.f;

    // View Space
    output[0].position = vtx.viewPos + float4(-scale, scale, 0.f, 0.f);
    output[1].position = vtx.viewPos + float4(scale, scale, 0.f, 0.f);
    output[2].position = vtx.viewPos + float4(scale, -scale, 0.f, 0.f);
    output[3].position = vtx.viewPos + float4(-scale, -scale, 0.f, 0.f);

    // Projection Space
    output[0].position = mul(output[0].position, PassParams.MatProjection);
    output[1].position = mul(output[1].position, PassParams.MatProjection);
    output[2].position = mul(output[2].position, PassParams.MatProjection);
    output[3].position = mul(output[3].position, PassParams.MatProjection);

    output[0].uv = float2(0.f, 0.f);
    output[1].uv = float2(1.f, 0.f);
    output[2].uv = float2(1.f, 1.f);
    output[3].uv = float2(0.f, 1.f);

    output[0].id = id;
    output[1].id = id;
    output[2].id = id;
    output[3].id = id;

    outputStream.Append(output[0]);
    outputStream.Append(output[1]);
    outputStream.Append(output[2]);
    outputStream.RestartStrip();

    outputStream.Append(output[0]);
    outputStream.Append(output[2]);
    outputStream.Append(output[3]);
    outputStream.RestartStrip();
}

float4 PS_Main(GS_OUT input) : SV_Target
{
    return TextureMaps[ParticleShared[GlobalParams.ParticleIndex].TextureIndex].Sample(g_sam_0, input.uv);
}






// CS_Main
// g_vec2_1 : DeltaTime / AccTime
// g_int_0  : Particle Max Count
// g_int_1  : AddCount
// g_vec4_0 : MinLifeTime / MaxLifeTime / MinSpeed / MaxSpeed
[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    if (threadIndex.x >= Particle[threadIndex.x].maxCount)
        return;

    int maxCount = Particle[threadIndex.x].maxCount;
    int addCount = Particle[threadIndex.x].addCount;
    int frameNumber = Particle[threadIndex.x].frameNumber;
    float deltaTime = Particle[threadIndex.x].deltaTime;
    float accTime = Particle[threadIndex.x].accTime;
    float minLifeTime = Particle[threadIndex.x].minLifeTime;
    float maxLifeTime = Particle[threadIndex.x].maxLifeTime;
    float minSpeed = Particle[threadIndex.x].minSpeed;
    float maxSpeed = Particle[threadIndex.x].maxSpeed;

    RWParticleShared[GlobalParams.ParticleIndex].addCount = addCount;
    GroupMemoryBarrierWithGroupSync();  //임계영역 보호를 위한 barrier (동기화)

    if (RWParticle[threadIndex.x].alive == 0)
    {
        while (true)
        {
            int remaining = RWParticleShared[GlobalParams.ParticleIndex].addCount;
            if (remaining <= 0)
                break;

            int expected = remaining;
            int desired = remaining - 1;
            int originalValue;
            InterlockedCompareExchange(RWParticleShared[GlobalParams.ParticleIndex].addCount, expected, desired, originalValue); //임계영역
                                                                                                //한번에 한번만 실행함
                                                                                                //if (addCount == expected )addCount = desired
                                                                                                // originalValue = addCount
            
            if (originalValue == expected)
            {
                RWParticle[threadIndex.x].alive = 1;
                break;
            }
        }

        if (RWParticle[threadIndex.x].alive == 1)
        {
            float x = ((float) threadIndex.x / (float) maxCount) + accTime;

            float r1 = Rand(float2(x, accTime));
            float r2 = Rand(float2(x * accTime, accTime));
            float r3 = Rand(float2(x * accTime * accTime, accTime * accTime));

            // [0.5~1] -> [0~1]
            float3 noise =
            {
                2 * r1 - 1,
                2 * r2 - 1,
                2 * r3 - 1
            };

            // [0~1] -> [-1~1]
            float3 dir = (noise - 0.5f) * 2.f;

            RWParticle[threadIndex.x].worldDir = normalize(dir);
            RWParticle[threadIndex.x].worldPos = (noise.xyz - 0.5f) * 25;
            RWParticle[threadIndex.x].lifeTime = ((maxLifeTime - minLifeTime) * noise.x) + minLifeTime;
            RWParticle[threadIndex.x].curTime = 0.f;
        }
    }
    else
    {
        RWParticle[threadIndex.x].curTime += deltaTime;
        if (RWParticle[threadIndex.x].lifeTime < RWParticle[threadIndex.x].curTime)
        {
            RWParticle[threadIndex.x].alive = 0;
            return;
        }

        float ratio = RWParticle[threadIndex.x].curTime / RWParticle[threadIndex.x].lifeTime;
        float speed = (maxSpeed - minSpeed) * ratio + minSpeed;
        RWParticle[threadIndex.x].worldPos += RWParticle[threadIndex.x].worldDir * speed * deltaTime;
    }
}

#endif