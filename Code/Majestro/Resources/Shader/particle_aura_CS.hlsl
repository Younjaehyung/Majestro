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

                float r1 = Rand01(float2(seed, shareds.accTime + 11.0f));
                float r2 = Rand01(float2(seed + 13.0f, shareds.accTime + 17.0f));
                float r3 = Rand01(float2(seed + 19.0f, shareds.accTime + 23.0f));
                float r4 = Rand01(float2(seed + 29.0f, shareds.accTime + 31.0f));

                const float innerRadius = 18.0f;
                const float outerRadius = 28.0f;
                const float angle = r1 * 6.2831853f;
                const bool outerRing = r4 > 0.35f;
                const float radius = outerRing ? lerp(innerRadius, outerRadius, r2) : sqrt(r2) * innerRadius;

                float3 radial = float3(cos(angle), 0.0f, sin(angle));
                float3 tangent = float3(-radial.z, 0.0f, radial.x);
                float spiralSign = (r3 > 0.5f) ? 1.0f : -1.0f;

                RWParticle[threadIndex.x].alive = 1;
                RWParticle[threadIndex.x].worldPos = radial * radius;
                RWParticle[threadIndex.x].worldDir = normalize(float3(radial.x * 0.15f + tangent.x * spiralSign * 0.5f, 1.0f, radial.z * 0.15f + tangent.z * spiralSign * 0.5f));
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
        float3 pos = RWParticle[threadIndex.x].worldPos;
        float2 flat = pos.xz;
        float flatLen = max(length(flat), 0.001f);
        float2 radial = flat / flatLen;
        float2 tangent = float2(-radial.y, radial.x);

        const float upSpeed = lerp(shareds.maxSpeed, shareds.minSpeed, ratio);
        const float outwardSpeed = lerp(1.0f, 7.0f, ratio);
        const float spiralSpeed = lerp(16.0f, 5.0f, ratio);

        float2 horizontal = radial * outwardSpeed + tangent * spiralSpeed;
        float3 velocity = float3(horizontal.x, upSpeed, horizontal.y) + RWParticle[threadIndex.x].worldDir * 4.0f;
        RWParticle[threadIndex.x].worldPos += velocity * shareds.deltaTime;
    }
}
