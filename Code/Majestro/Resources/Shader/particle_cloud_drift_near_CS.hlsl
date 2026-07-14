#include "params.hlsl"
#include "utils.hlsl"

[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    const uint emitterIndex = GlobalParams.PassScalar0;
    const PARTICLESHARED shareds = ParticleShared[emitterIndex];

    if (threadIndex.x >= shareds.maxCount)
        return;

    const float3 windDir = normalize(float3(-0.78f, 0.0f, 0.62f));
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
                float seed = ((float) threadIndex.x + 1.0f) * 0.193f + PassParams.TotalTime * 0.91f;
                float r1 = Rand01(float2(seed, seed + 19.0f));
                float r2 = Rand01(float2(seed + 29.0f, seed + 37.0f));
                float r3 = Rand01(float2(seed + 41.0f, seed + 53.0f));
                float r4 = Rand01(float2(seed + 67.0f, seed + 71.0f));

                float startDistance = lerp(1500.0f, 3000.0f, r1);
                float sideOffset = lerp(-3200.0f, 3200.0f, r2);
                float height = lerp(-260.0f, 560.0f, r3);
                float depthOffset = lerp(-900.0f, 900.0f, r4);

                // Near cloud drift uses a smaller spawn volume and fewer larger chunks.
                // It gives the airship an occasional close pass without filling the whole screen.
                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos =
                    -windDir * startDistance +
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
            sin(PassParams.TotalTime * 0.28f + (float)threadIndex.x * 0.71f) *
            lerp(14.0f, 44.0f, RWParticle[threadIndex.x].worldDir.y);
        const float heightWave =
            sin(PassParams.TotalTime * 0.18f + (float)threadIndex.x * 0.43f) *
            lerp(7.0f, 24.0f, RWParticle[threadIndex.x].worldDir.z);

        float3 velocity = windDir * speed;
        velocity += sideDir * sideWave;
        velocity += float3(0.0f, heightWave, 0.0f);

        RWParticle[threadIndex.x].worldPos += velocity * shareds.deltaTime;
    }
}
