#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"





[numthreads(1, 4, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{

    uint rel = threadIdx.y;
    uint inst = GlobalParams.BaseInstanceID + rel;
    
    
    if (rel >= GlobalParams.etc)
        return;
    ANIMINSTANCE animationInst = AnimInstance[inst];
    ANIMATIONMETA animationclipmeta = AnimationMeta[animationInst.AnimClipIdx];
    uint boneidx = animationclipmeta.BoneStart;
    uint bonecount = animationclipmeta.BoneCount;
    

    
    const float featherRange = 4.0f;


     // 1) 로컬 본 포즈 계산 + 부모 누적(모델 공간) 저장
    for (uint nowbone = 0; nowbone < bonecount; ++nowbone)
    {

        float4 lowerScale, lowerRotation, lowerTranslation;
        SampleAnimation(
            nowbone,
            animationclipmeta.NumFrame,
            animationInst.CurrentFrame,
            animationInst.NextFrame,
            animationInst.Ratio,
            animationclipmeta.AnimOffset,
            lowerScale,
            lowerRotation,
            lowerTranslation
        );

        // 하체 Base 레이어에 블렌드 애니메이션이 있으면 블렌드한다.
        if (animationInst.BlendWeight > 0.0001f && animationInst.BlendClipIdx != animationInst.AnimClipIdx)
        {
            ANIMATIONMETA blendMeta = AnimationMeta[animationInst.BlendClipIdx];
            if (nowbone < blendMeta.BoneCount)
            {
                float4 blendScale, blendRotation, blendTranslation;
                SampleAnimation(
                    nowbone,
                    blendMeta.NumFrame,
                    animationInst.BlendCurrentFrame,
                    animationInst.BlendNextFrame,
                    animationInst.BlendRatio,
                    blendMeta.AnimOffset,
                    blendScale,
                    blendRotation,
                    blendTranslation
                );

                float finalBlendWeight = saturate(animationInst.BlendWeight);

                if (animationInst.BlendMode == 1)
                {
                    const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
                    const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

                    float4 additiveScale = lerp(identityScale, blendScale, finalBlendWeight);
                    lowerScale.xyz *= additiveScale.xyz;

                    float4 additiveRotation = QuaternionSlerp(identityRotation, blendRotation, finalBlendWeight);
                    lowerRotation = QuaternionMultiply(lowerRotation, additiveRotation);

                    lowerTranslation.xyz += blendTranslation.xyz * finalBlendWeight;
                }
                else
                {
                    lowerScale = lerp(lowerScale, blendScale, finalBlendWeight);
                    lowerRotation = QuaternionSlerp(lowerRotation, blendRotation, finalBlendWeight);
                    lowerTranslation = lerp(lowerTranslation, blendTranslation, finalBlendWeight);
                }
            }

        }
            

        float4 finalScale = lowerScale;
        float4 finalRotation = lowerRotation;
        float4 finalTranslation = lowerTranslation;

        // 상체 Upper 레이어가 있고, Upper 레이어의 애니메이션 클립이 Base 레이어와 다르면 
        // Upper 레이어 애니메이션을 샘플링해서 블렌드한다.
        if (animationInst.UpperLayerWeight > 0.0001f &&
            animationInst.UpperAnimClipIdx != animationInst.AnimClipIdx)
        {
            ANIMATIONMETA upperMeta = AnimationMeta[animationInst.UpperAnimClipIdx];

            if (nowbone < upperMeta.BoneCount)
            {
                float4 upperScale, upperRotation, upperTranslation;
                SampleAnimation(
                    nowbone,
                    upperMeta.NumFrame,
                    animationInst.UpperCurrentFrame,
                    animationInst.UpperNextFrame,
                    animationInst.UpperRatio,
                    upperMeta.AnimOffset,
                    upperScale,
                    upperRotation,
                    upperTranslation
                );

                if (animationInst.UpperBlendWeight > 0.0001f &&
                    animationInst.UpperBlendClipIdx != animationInst.UpperAnimClipIdx)
                {
                    ANIMATIONMETA upperBlendMeta = AnimationMeta[animationInst.UpperBlendClipIdx];
                    if (nowbone < upperBlendMeta.BoneCount)
                    {
                        float4 upperBlendScale, upperBlendRotation, upperBlendTranslation;
                        SampleAnimation(
                            nowbone,
                            upperBlendMeta.NumFrame,
                            animationInst.UpperBlendCurrentFrame,
                            animationInst.UpperBlendNextFrame,
                            animationInst.UpperBlendRatio,
                            upperBlendMeta.AnimOffset,
                            upperBlendScale,
                            upperBlendRotation,
                            upperBlendTranslation
                        );

                        float upperBlendWeight = saturate(animationInst.UpperBlendWeight);
                        upperScale = lerp(upperScale, upperBlendScale, upperBlendWeight);
                        upperRotation = QuaternionSlerp(upperRotation, upperBlendRotation, upperBlendWeight);
                        upperTranslation = lerp(upperTranslation, upperBlendTranslation, upperBlendWeight);
                    }
                }
                  // 하체 기준 고정:
                // Upper 레이어의 페더가 마스크 시작점 아래(하체 체인)로 내려오지 않도록 차단한다.
                // 이렇게 해야 상체 보간이 하체(루트/골반) 방향을 끌고 가지 않는다.
                float upperBlendWeight = 0.0f;
                if (nowbone >= animationInst.UpperMaskStart)
                {
                    upperBlendWeight = CalculateBlendWeight(
                    nowbone,
                    animationInst.UpperMaskStart,
                    animationInst.UpperMaskEnd,
                    featherRange
                );
                }
                float boneWeight = SkeletonBone[nowbone + boneidx].BlendWeight;
                // pelvis/leg 구간은 Upper 영향 완전 차단 (하체 기준 유지)
                //if (boneWeight > 0.7f)
                //    boneWeight = 1.0f;
                float finalUpperWeight = saturate(animationInst.UpperLayerWeight) *
                                         saturate(boneWeight) *
                                         upperBlendWeight;

                if (finalUpperWeight > 0.999f)
                {
                    finalScale = upperScale;
                    finalRotation = upperRotation;
                    finalTranslation = upperTranslation;
                }
                else if (finalUpperWeight > 0.0001f)
                {
                    if (animationInst.UpperBlendMode == 1)
                    {
                        const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
                        const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

                        float4 deltaRotation = QuaternionMultiply(upperRotation, QuaternionConjugate(lowerRotation));
                        float4 additiveRotation = QuaternionSlerp(identityRotation, deltaRotation, finalUpperWeight);
                        finalRotation = QuaternionMultiply(lowerRotation, additiveRotation);

                        float4 deltaScale = upperScale / max(lowerScale, float4(0.0001f, 0.0001f, 0.0001f, 0.0001f));
                        float4 additiveScale = lerp(identityScale, deltaScale, finalUpperWeight);
                        finalScale = lowerScale * additiveScale;

                        float4 deltaTranslation = upperTranslation - lowerTranslation;
                        finalTranslation = lowerTranslation + deltaTranslation * finalUpperWeight;
                    }
                    else
                    {
                        finalScale = lerp(lowerScale, upperScale, finalUpperWeight);
                        finalRotation = QuaternionSlerp(lowerRotation, upperRotation, finalUpperWeight);
                        finalTranslation = lerp(lowerTranslation, upperTranslation, finalUpperWeight);
                    }
                }
            }
        }
        finalRotation = normalize(finalRotation);
        finalScale = max(finalScale, float4(0.0001f, 0.0001f, 0.0001f, 1.0f));

        float4 quaternionzero = float4(0.f, 0.f, 0.f, 1.f);
        matrix localBone = MatrixAffineTransformation(finalScale, quaternionzero, finalRotation, finalTranslation);

        int parentIdx = SkeletonBone[nowbone + boneidx].ParentIdx;
        matrix modelBone = localBone;
        if (parentIdx >= 0 && parentIdx < int(bonecount))
        {
            matrix parentModel = RFinalBone[animationInst.ReulstIndex + (uint) parentIdx];
            modelBone = mul(localBone, parentModel);
        }

        RFinalBone[animationInst.ReulstIndex + nowbone] = modelBone;
        
           
    }
    
    for (uint nowbone = 0; nowbone < bonecount; ++nowbone)
    {
        matrix modelBone = RFinalBone[animationInst.ReulstIndex + nowbone];
        RFinalBone[animationInst.ReulstIndex + nowbone] = mul(SkeletonBone[nowbone + boneidx].Offset, modelBone);
    }

}