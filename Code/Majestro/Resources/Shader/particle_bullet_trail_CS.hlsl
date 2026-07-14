#include "params.hlsl"
#include "utils.hlsl"


float3 BulletTrailSafeNormalize(float3 value, float3 fallback)
{
    float lenSq = dot(value, value);
    return (lenSq > 0.0001f) ? value * rsqrt(lenSq) : fallback;
}

float3 BulletTrailForward()
{
    float3 forward = -float3(ParticleShared[GlobalParams.PassScalar0].MatWorld._31, ParticleShared[GlobalParams.PassScalar0].MatWorld._32, ParticleShared[GlobalParams.PassScalar0].MatWorld._33);
    return BulletTrailSafeNormalize(forward, float3(0.0f, 0.0f, -1.0f));
}

[numthreads(1024, 1, 1)]
void CS_Main(int3 threadIndex : SV_DispatchThreadID)
{
    const uint emitterIndex = GlobalParams.PassScalar0;
    const PARTICLESHARED shareds = ParticleShared[emitterIndex];

    if (threadIndex.x >= shareds.maxCount)
        return;

    const float3 forward = BulletTrailForward();
    const float3 up = float3(0.0f, 1.0f, 0.0f);
    const float3 right = BulletTrailSafeNormalize(cross(up, forward), float3(1.0f, 0.0f, 0.0f));
    const float3 localUp = BulletTrailSafeNormalize(cross(forward, right), up);

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

                float r1 = Rand01(float2(seed, shareds.accTime + 71.0f));
                float r2 = Rand01(float2(seed + 73.0f, shareds.accTime + 79.0f));
                float r3 = Rand01(float2(seed + 83.0f, shareds.accTime + 89.0f));

                const float trailLength = lerp(8.0f, 82.0f, r1);
                const float width = lerp(0.8f, 4.0f, r2);
                const float side = (r2 - 0.5f) * width;
                const float height = (r3 - 0.5f) * width;

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos = -forward * trailLength + right * side + localUp * height;
                RWParticle[threadIndex.x].worldDir = forward;
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
        const float dragBack = lerp(shareds.maxSpeed, shareds.minSpeed, ratio);
        const float spread = lerp(0.0f, 8.0f, ratio);
        float3 trailDir = BulletTrailSafeNormalize(RWParticle[threadIndex.x].worldDir, forward);

        RWParticle[threadIndex.x].worldPos += (-trailDir * dragBack + right * spread * 0.25f) * shareds.deltaTime;
    }
}
