#include "params.hlsl"
#include "utils.hlsl"

[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    const uint emitterIndex = GlobalParams.PassScalar0;
    const PARTICLESHARED shareds = ParticleShared[emitterIndex];

    if (threadIndex.x >= shareds.maxCount)
        return;


    const float3 windDir = float3(0.0f, 0.0f, -1.0f);
    const float3 sideDir = normalize(float3(-windDir.z, 0.0f, windDir.x));

    if (RWParticle[threadIndex.x].alive == 0)
    {
        while (true)
        {
            int remaining = RWParticleShared[emitterIndex].addCount;
            if (remaining <= 0)
                break;

            int originalValue = 0;
            InterlockedCompareExchange(
                RWParticleShared[emitterIndex].addCount,
                remaining,
                remaining - 1,
                originalValue);

            if (originalValue == remaining)
            {
                float seed = ((float) threadIndex.x + 1.0f) * 0.137f + PassParams.TotalTime * 0.73f;
                float r1 = Rand01(float2(seed, seed + 17.0f));
                float r2 = Rand01(float2(seed + 23.0f, seed + 31.0f));
                float r3 = Rand01(float2(seed + 43.0f, seed + 47.0f));
                float r4 = Rand01(float2(seed + 59.0f, seed + 61.0f));

            
                const float kScatter = 1700.0f;
                const float kSpawnAhead = 4400.0f;

                float sideOffset = lerp(-kScatter, kScatter, r2);
                float height = lerp(-kScatter, kScatter, r3);
                float depthOffset = lerp(-kScatter, kScatter, r4);

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos =
                    -windDir * kSpawnAhead +
                    sideDir * sideOffset +
                    float3(0.0f, height, 0.0f) +
                    windDir * depthOffset;
                RWParticle[threadIndex.x].worldDir = float3(r1, r2, r4);
                RWParticle[threadIndex.x].lifeTime = lerp(shareds.minLifeTime, shareds.maxLifeTime, r4);
                RWParticle[threadIndex.x].curTime = 0.0f;
                break;
            }
        }
    }
    else
    {
        RWParticle[threadIndex.x].curTime += shareds.deltaTime;
        if (RWParticle[threadIndex.x].lifeTime <= RWParticle[threadIndex.x].curTime)
        {
            RWParticle[threadIndex.x].alive = 0;
            return;
        }

        const float speed = lerp(shareds.minSpeed, shareds.maxSpeed, RWParticle[threadIndex.x].worldDir.x);
        const float sideWave =
            sin(PassParams.TotalTime * 0.35f + (float) threadIndex.x * 0.61f) *
            lerp(8.0f, 32.0f, RWParticle[threadIndex.x].worldDir.y);
        const float heightWave =
            sin(PassParams.TotalTime * 0.22f + (float) threadIndex.x * 0.37f) *
            lerp(4.0f, 18.0f, RWParticle[threadIndex.x].worldDir.z);

        float3 velocity = windDir * speed;
        velocity += sideDir * sideWave;
        velocity += float3(0.0f, heightWave, 0.0f);

        RWParticle[threadIndex.x].worldPos += velocity * shareds.deltaTime;
    }
}
