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
                float seed = ((float) threadIndex.x / max(1, shareds.maxCount)) + shareds.accTime;

                float r1 = Rand01(float2(seed, shareds.accTime + 41.0f));
                float r2 = Rand01(float2(seed + 43.0f, shareds.accTime + 47.0f));
                float r3 = Rand01(float2(seed + 53.0f, shareds.accTime + 59.0f));
                float r4 = Rand01(float2(seed + 61.0f, shareds.accTime + 67.0f));

                const float angle = r1 * 6.2831853f;
                const float radius = lerp(42.0f, 76.0f, r2);
                const float height = lerp(28.0f, 95.0f, r3);
                const float spinSign = (r4 > 0.5f) ? 1.0f : -1.0f;

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos = float3(cos(angle) * radius, height, sin(angle) * radius);
                RWParticle[threadIndex.x].worldDir = float3(spinSign, r3, r4);
                RWParticle[threadIndex.x].lifeTime = lerp(shareds.minLifeTime, shareds.maxLifeTime, saturate(r4));
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
        const float spinSign = (RWParticle[threadIndex.x].worldDir.x >= 0.0f) ? 1.0f : -1.0f;
        const float randomBand = saturate(RWParticle[threadIndex.x].worldDir.y);
        const float spinSpeed = lerp(shareds.minSpeed, shareds.maxSpeed, randomBand) * 0.035f * spinSign;
        const float angleStep = spinSpeed * shareds.deltaTime;

        float3 pos = RWParticle[threadIndex.x].worldPos;
        float c = cos(angleStep);
        float s = sin(angleStep);
        float2 rotated = float2(pos.x * c - pos.z * s, pos.x * s + pos.z * c);

        const float baseHeight = lerp(32.0f, 88.0f, randomBand);
        const float bobPhase = (float) threadIndex.x * 0.37f + RWParticle[threadIndex.x].curTime * 3.0f;
        const float bobHeight = sin(bobPhase) * lerp(8.0f, 22.0f, saturate(RWParticle[threadIndex.x].worldDir.z));

        RWParticle[threadIndex.x].worldPos = float3(rotated.x, baseHeight + bobHeight, rotated.y);
    }
}
