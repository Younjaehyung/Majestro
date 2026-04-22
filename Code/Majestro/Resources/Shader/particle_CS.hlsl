#include "params.hlsl"
#include "utils.hlsl"

[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    const uint emitterIndex = GlobalParams.etc;
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
                float x = ((float) threadIndex.x / max(1, shareds.maxCount)) + shareds.accTime;

                float r1 = Rand(float2(x, shareds.accTime));
                float r2 = Rand(float2(x * shareds.accTime + 1.0f, shareds.accTime + 2.0f));
                float r3 = Rand(float2(x * shareds.accTime + 3.0f, shareds.accTime + 4.0f));

                float3 noise = float3(2.0f * r1 - 1.0f, 2.0f * r2 - 1.0f, 2.0f * r3 - 1.0f);
                float3 dir = normalize(max(abs(noise.x) + abs(noise.y) + abs(noise.z), 0.001f) * noise);

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldDir = dir;
                RWParticle[threadIndex.x].worldPos = noise * 25.0f;
                RWParticle[threadIndex.x].lifeTime = lerp(shareds.minLifeTime, shareds.maxLifeTime, saturate(r1));
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
        const float speed = lerp(shareds.minSpeed, shareds.maxSpeed, ratio);
        RWParticle[threadIndex.x].worldPos += RWParticle[threadIndex.x].worldDir * speed * shareds.deltaTime;
    }
}
