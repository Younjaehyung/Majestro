#include "params.hlsl"
#include "utils.hlsl"


[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    const uint emitterIndex = GlobalParams.PassScalar0;
    const PARTICLESHARED shareds = ParticleShared[emitterIndex];

    if (threadIndex.x >= shareds.maxCount)
        return;

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
                float seed = ((float) threadIndex.x / max(1, shareds.maxCount)) + shareds.accTime;

                float r1 = Rand01(float2(seed, shareds.accTime + 1.0f));
                float r2 = Rand01(float2(seed + 2.0f, shareds.accTime + 3.0f));
                float r3 = Rand01(float2(seed + 4.0f, shareds.accTime + 5.0f));
                float r4 = Rand01(float2(seed + 6.0f, shareds.accTime + 7.0f));

                const float baseRadius = 18.0f;
                const float angle = r1 * 6.2831853f;
                const float radius = sqrt(r2) * baseRadius;

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos = float3(cos(angle) * radius, 0.0f, sin(angle) * radius);
                RWParticle[threadIndex.x].worldDir = normalize(float3((r3 - 0.5f) * 0.45f, 1.0f, (r4 - 0.5f) * 0.45f));
                RWParticle[threadIndex.x].lifeTime = lerp(shareds.minLifeTime, shareds.maxLifeTime, saturate(r3));
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

        const float ratio = saturate(RWParticle[threadIndex.x].curTime / max(RWParticle[threadIndex.x].lifeTime, 0.0001f));
        const float riseSpeed = lerp(shareds.maxSpeed, shareds.minSpeed, ratio);
        const float swaySeed = (float) threadIndex.x * 0.173f;
        const float swayX = sin(shareds.accTime * 2.2f + swaySeed) * 3.0f;
        const float swayZ = cos(shareds.accTime * 1.7f + swaySeed) * 3.0f;
        const float3 velocity = float3(swayX, riseSpeed, swayZ) + RWParticle[threadIndex.x].worldDir * riseSpeed * 0.25f;

        RWParticle[threadIndex.x].worldPos += velocity * shareds.deltaTime;
    }
}
