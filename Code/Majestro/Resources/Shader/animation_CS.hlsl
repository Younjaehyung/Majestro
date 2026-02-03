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
    


[numthreads(128, 4, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    uint nowbone = threadIdx.x;
    uint rel = threadIdx.y; // 현재 인스턴스 idx   | GlobalParams.BaseInstanceID : 전체 인스턴스 idx
    uint inst = GlobalParams.BaseInstanceID + rel;
    ANIMATIONMETA animationclipmeta = AnimationMeta[AnimInstance[inst].AnimClipIdx];
    ANIMINSTANCE animationInst = AnimInstance[inst];
    
    uint boneidx = animationclipmeta.BoneStart;
    uint bonecount = animationclipmeta.BoneCount;
    uint framecount = animationclipmeta.NumFrame;
    
    uint currentframe = animationInst.CurrentFrame;
    uint nextframe = animationInst.NextFrame;
    float ratio = animationInst.Ratio;
   
    if (nowbone >= bonecount || rel >= GlobalParams.etc)
        return; //etc : 현재 애니메이션을 사용하는 인스턴스 수
        
  
    
    uint idx = nowbone * framecount + currentframe + animationclipmeta.AnimOffset;
    uint nextidx = nowbone * framecount + nextframe + animationclipmeta.AnimOffset;

    float4 quaternionzero = float4(0.f, 0.f, 0.f, 1.f);

    
    
    float4 scale = lerp(AnimationClip[idx].Scale, AnimationClip[nextidx].Scale, ratio);
    float4 rotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextidx].Rotation, ratio);
    float4 translation = lerp(AnimationClip[idx].Translation, AnimationClip[nextidx].Translation, ratio);

    float blendWeight = saturate(animationInst.BlendWeight);
    if (animationInst.BlendMaskEnd > animationInst.BlendMaskStart)
    {
        const bool inMask = nowbone >= animationInst.BlendMaskStart && nowbone <= animationInst.BlendMaskEnd;
        blendWeight *= inMask ? 1.0f : 0.0f;
    }
    if (blendWeight > 0.0001f && animationInst.BlendClipIdx != animationInst.AnimClipIdx)
    {
        ANIMATIONMETA blendMeta = AnimationMeta[animationInst.BlendClipIdx];
        uint blendFrameCount = blendMeta.NumFrame;
        uint blendIdx = nowbone * blendFrameCount + animationInst.BlendCurrentFrame + blendMeta.AnimOffset;
        uint blendNextIdx = nowbone * blendFrameCount + animationInst.BlendNextFrame + blendMeta.AnimOffset;

        float4 blendScale = lerp(AnimationClip[blendIdx].Scale, AnimationClip[blendNextIdx].Scale, animationInst.BlendRatio);
        float4 blendRotation = QuaternionSlerp(AnimationClip[blendIdx].Rotation, AnimationClip[blendNextIdx].Rotation, animationInst.BlendRatio);
        float4 blendTranslation = lerp(AnimationClip[blendIdx].Translation, AnimationClip[blendNextIdx].Translation, animationInst.BlendRatio);

        scale = lerp(scale, blendScale, blendWeight);
        rotation = QuaternionSlerp(rotation, blendRotation, blendWeight);
        translation = lerp(translation, blendTranslation, blendWeight);
    }

    
    matrix matbone = MatrixAffineTransformation(scale, quaternionzero, rotation, translation);
    
    //if (IsExactIdentity(matbone))
    //    return;
    
    RFinalBone[animationInst.ReulstIndex + nowbone] = mul(SkeletonBone[nowbone + boneidx], matbone);

}
