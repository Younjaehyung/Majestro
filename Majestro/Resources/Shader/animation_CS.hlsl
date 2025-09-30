#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

// ComputeAnimation
// StructuredBuffer<matrix> SkeletonBone : register(t0, space3);
// StructuredBuffer<ANIMFRAMEPARAMS> AnimationClip : register(t1, space3);
// StructuredBuffer<ANIMATIONMETA> AnimationMeta : register(t2, space3);
// StructuredBuffer<Matrix> SFinalBone : register(t4, space1);
// RWStructuredBuffer<Matrix> RFinalBone : register(u0, space1);



//uint Index0 : animationClip;
//uint Index1 : currentFrame;
//uint Index2 : nextFrame;
//uint Index3 : ratioInt;
    


[numthreads(256, 1, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    //ANIMATIONMETA animationClipMeta = AnimationMeta[GlobalParams.Index0]; // To-Do
    
    //uint nowBone = threadIdx.x;
    //if (nowBone >= animationClipMeta.BoneCount)
    //    return;

    
    //uint boneCount = animationClipMeta.BoneCount;
    //uint currentFrame = GlobalParams.Index1;
    //uint nextFrame = GlobalParams.Index2;
    //float ratio = GlobalParams.Index3;

    //uint idx = (30 * nowBone) + currentFrame;
    //uint nextIdx = (30 * nowBone) + nextFrame;

    //float4 quaternionZero = float4(0.f, 0.f, 0.f, 1.f);

    
    
    //float4 scale = lerp(AnimationClip[idx].Scale, AnimationClip[nextIdx].Scale, ratio);
    //float4 rotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextIdx].Rotation, ratio);
    //float4 translation = lerp(AnimationClip[idx].Translation, AnimationClip[nextIdx].Translation, ratio);

    //matrix matBone = MatrixAffineTransformation(scale, quaternionZero, rotation, translation);

    //RFinalBone[threadIdx.x] = mul(SkeletonBone[threadIdx.x], matBone);

}
