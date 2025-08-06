
#include "params.hlsl"
#include "utils.hlsl"

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
