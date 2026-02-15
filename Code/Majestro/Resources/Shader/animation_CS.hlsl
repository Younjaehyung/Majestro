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
    
float4 QuaternionConjugate(float4 q)
{
    return float4(-q.xyz, q.w);
}


float4 QuaternionMultiply(float4 q1, float4 q2)
{
    return normalize(float4(
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z));
}

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

    
    
    float4 lowerScale = lerp(AnimationClip[idx].Scale, AnimationClip[nextidx].Scale, ratio);
    float4 lowerRotation = QuaternionSlerp(AnimationClip[idx].Rotation, AnimationClip[nextidx].Rotation, ratio);
    float4 lowerTranslation = lerp(AnimationClip[idx].Translation, AnimationClip[nextidx].Translation, ratio);
    float lowerBlendWeight = saturate(animationInst.BlendWeight);
    
    if (animationInst.BlendMaskEnd > animationInst.BlendMaskStart)
    {
        const float feather = 2.0f;
        const float b = (float) nowbone;
        const float s = (float) animationInst.BlendMaskStart;
        const float e = (float) animationInst.BlendMaskEnd;

        const float rise = saturate((b - (s - feather)) / feather);
        const float fall = saturate(((e + feather) - b) / feather);
        lowerBlendWeight *= rise * fall;
    }
    if (lowerBlendWeight > 0.0001f && animationInst.BlendClipIdx != animationInst.AnimClipIdx)
    {
        ANIMATIONMETA blendMeta = AnimationMeta[animationInst.BlendClipIdx];
        if (nowbone < blendMeta.BoneCount)
        {
            uint blendFrameCount = blendMeta.NumFrame;
            uint blendIdx = nowbone * blendFrameCount + animationInst.BlendCurrentFrame + blendMeta.AnimOffset;
            uint blendNextIdx = nowbone * blendFrameCount + animationInst.BlendNextFrame + blendMeta.AnimOffset;
            
            float4 blendScale = lerp(AnimationClip[blendIdx].Scale, AnimationClip[blendNextIdx].Scale, animationInst.BlendRatio);
            float4 blendRotation = QuaternionSlerp(AnimationClip[blendIdx].Rotation, AnimationClip[blendNextIdx].Rotation, animationInst.BlendRatio);
            float4 blendTranslation = lerp(AnimationClip[blendIdx].Translation, AnimationClip[blendNextIdx].Translation, animationInst.BlendRatio);
            if (animationInst.BlendMode == 1)
            {
                const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
                const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

                float4 additiveScale = lerp(identityScale, blendScale, lowerBlendWeight);
                lowerScale.xyz *= additiveScale.xyz;

                float4 additiveRotation = QuaternionSlerp(identityRotation, blendRotation, lowerBlendWeight);
                lowerRotation = QuaternionMultiply(lowerRotation, additiveRotation);

                lowerTranslation.xyz += blendTranslation.xyz * lowerBlendWeight;
            }
            else
            {
                lowerScale = lerp(lowerScale, blendScale, lowerBlendWeight);
                lowerRotation = QuaternionSlerp(lowerRotation, blendRotation, lowerBlendWeight);
                lowerTranslation = lerp(lowerTranslation, blendTranslation, lowerBlendWeight);
            }
        }
    }
    ANIMATIONMETA upperMeta = AnimationMeta[animationInst.UpperAnimClipIdx];
    float4 upperScale = lowerScale;
    float4 upperRotation = lowerRotation;
    float4 upperTranslation = lowerTranslation;

    if (nowbone < upperMeta.BoneCount)
    {
        uint upperFrameCount = upperMeta.NumFrame;
        uint upperIdx = nowbone * upperFrameCount + animationInst.UpperCurrentFrame + upperMeta.AnimOffset;
        uint upperNextIdx = nowbone * upperFrameCount + animationInst.UpperNextFrame + upperMeta.AnimOffset;

        upperScale = lerp(AnimationClip[upperIdx].Scale, AnimationClip[upperNextIdx].Scale, animationInst.UpperRatio);
        upperRotation = QuaternionSlerp(AnimationClip[upperIdx].Rotation, AnimationClip[upperNextIdx].Rotation, animationInst.UpperRatio);
        upperTranslation = lerp(AnimationClip[upperIdx].Translation, AnimationClip[upperNextIdx].Translation, animationInst.UpperRatio);
    }

    float upperBlendWeight = saturate(animationInst.UpperBlendWeight);
    if (upperBlendWeight > 0.0001f && animationInst.UpperBlendClipIdx != animationInst.UpperAnimClipIdx)
    {
        ANIMATIONMETA upperBlendMeta = AnimationMeta[animationInst.UpperBlendClipIdx];
        if (nowbone < upperBlendMeta.BoneCount)
        {
            uint upperBlendFrameCount = upperBlendMeta.NumFrame;
            uint upperBlendIdx = nowbone * upperBlendFrameCount + animationInst.UpperBlendCurrentFrame + upperBlendMeta.AnimOffset;
            uint upperBlendNextIdx = nowbone * upperBlendFrameCount + animationInst.UpperBlendNextFrame + upperBlendMeta.AnimOffset;

            float4 upperBlendScale = lerp(AnimationClip[upperBlendIdx].Scale, AnimationClip[upperBlendNextIdx].Scale, animationInst.UpperBlendRatio);
            float4 upperBlendRotation = QuaternionSlerp(AnimationClip[upperBlendIdx].Rotation, AnimationClip[upperBlendNextIdx].Rotation, animationInst.UpperBlendRatio);
            float4 upperBlendTranslation = lerp(AnimationClip[upperBlendIdx].Translation, AnimationClip[upperBlendNextIdx].Translation, animationInst.UpperBlendRatio);

            upperScale = lerp(upperScale, upperBlendScale, upperBlendWeight);
            upperRotation = QuaternionSlerp(upperRotation, upperBlendRotation, upperBlendWeight);
            upperTranslation = lerp(upperTranslation, upperBlendTranslation, upperBlendWeight);
        }
    }

    float upperBoneWeight = saturate(SkeletonBone[nowbone + boneidx].BlendWeight);
    if (animationInst.UpperMaskEnd > animationInst.UpperMaskStart)
    {
        const float feather = 2.0f;
        const float b = (float) nowbone;
        const float s = (float) animationInst.UpperMaskStart;
        const float e = (float) animationInst.UpperMaskEnd;

        const float rise = saturate((b - (s - feather)) / feather);
        const float fall = saturate(((e + feather) - b) / feather);
        upperBoneWeight *= rise * fall;
    }

    const float upperApplyWeight = saturate(animationInst.UpperLayerWeight) * upperBoneWeight;
    if (upperApplyWeight > 0.0001f)
    {
        // 연결부 안정성을 위해 upper pose를 lower pose에 직접 보간한다.
        // Additive 모드도 현재는 안정성 우선으로 회전 중심의 약한 보간을 사용한다.
        if (animationInst.UpperBlendMode == 1)
        {
            lowerRotation = QuaternionSlerp(lowerRotation, upperRotation, upperApplyWeight);
            lowerTranslation = lerp(lowerTranslation, upperTranslation, upperApplyWeight * 0.35f);
            lowerScale = lerp(lowerScale, upperScale, upperApplyWeight * 0.2f);
        }
        else
        {
            lowerScale = lerp(lowerScale, upperScale, upperApplyWeight);
            lowerRotation = QuaternionSlerp(lowerRotation, upperRotation, upperApplyWeight);
            lowerTranslation = lerp(lowerTranslation, upperTranslation, upperApplyWeight);
        }
    }

    matrix matbone = MatrixAffineTransformation(lowerScale, quaternionzero, lowerRotation, lowerTranslation);
    //if (IsExactIdentity(matbone))
    //    return;
    
    RFinalBone[animationInst.ReulstIndex + nowbone] = mul(SkeletonBone[nowbone + boneidx].Offset, matbone);


}
