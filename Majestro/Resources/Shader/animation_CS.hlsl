#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

// ComputeAnimation
// StructuredBuffer<matrix> SkeletonBone : register(t0, space3);
// StructuredBuffer<ANIMFRAMEPARAMS> AnimationClip : register(t1, space3);
// StructuredBuffer<ANIMATIONMETA> AnimationMeta : register(t2, space3);
// StructuredBuffer<Matrix> SFinalBone : register(t4, space1);
// RWStructuredBuffer<Matrix> RFinalBone : register(u0, space1);
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    ANIMATIONMETA animationClipMeta = AnimationMeta[1]; // To-Do
    
    uint nowBone = threadIdx.x;
    if (nowBone >= animationClipMeta.BoneCount)
        return;

    
    uint boneCount = animationClipMeta.BoneCount;
    uint currentFrame = GlobalParams.ObjectIndex;
    uint nextFrame = GlobalParams.MaterialInfoIndex;
    uint ratioInt = GlobalParams.LightIndex;

    float ratio = (float) (ratioInt / 100);
    
    uint idx = (boneCount * currentFrame) + threadIdx.x;
    uint nextIdx = (boneCount * nextFrame) + threadIdx.x;

    float4 quaternionZero = float4(0.f, 0.f, 0.f, 1.f);

    float4 scale = lerp(AnimationClip[idx].Scale, AnimationClip[nextIdx].Scale, ratio);
    float4 rotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextIdx].Rotation, ratio);
    float4 translation = lerp(AnimationClip[idx].Translation, AnimationClip[nextIdx].Translation, ratio);

    matrix matBone = MatrixAffineTransformation(scale, quaternionZero, rotation, translation);

    RFinalBone[threadIdx.x] = mul(SkeletonBone[threadIdx.x], matBone);

}
