#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"





[numthreads(128, 4, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    uint nowbone = threadIdx.x;
    uint rel = threadIdx.y;
    uint inst = GlobalParams.BaseInstanceID + rel;
    
    ANIMATIONMETA animationclipmeta = AnimationMeta[AnimInstance[inst].AnimClipIdx];
    ANIMINSTANCE animationInst = AnimInstance[inst];
    
    uint boneidx = animationclipmeta.BoneStart;
    uint bonecount = animationclipmeta.BoneCount;
    
    if (nowbone >= bonecount || rel >= GlobalParams.etc)
        return;
    
    const float featherRange = 2.0f;
    
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
            
            if (animationInst.BlendMode == 1) // Additive
            {
                const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
                const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);
                
                float4 additiveScale = lerp(identityScale, blendScale, finalBlendWeight);
                lowerScale.xyz *= additiveScale.xyz;
                
                float4 additiveRotation = QuaternionSlerp(identityRotation, blendRotation, finalBlendWeight);
                lowerRotation = QuaternionMultiply(lowerRotation, additiveRotation);
                
                lowerTranslation.xyz += blendTranslation.xyz * finalBlendWeight;
            }
            else // Override
            {
                lowerScale = lerp(lowerScale, blendScale, finalBlendWeight);
                lowerRotation = QuaternionSlerp(lowerRotation, blendRotation, finalBlendWeight);
                lowerTranslation = lerp(lowerTranslation, blendTranslation, finalBlendWeight);
            }
        }
    }
    
    // UPPER BODY 애니메이션 처리
    float4 finalScale = lowerScale;
    float4 finalRotation = lowerRotation;
    float4 finalTranslation = lowerTranslation;
    
    // Upper 레이어가 활성화되고, Upper와 Lower가 다른 애니메이션일 때
    if (animationInst.UpperLayerWeight > 0.0001f &&
        animationInst.UpperAnimClipIdx != animationInst.AnimClipIdx)
    {
        ANIMATIONMETA upperMeta = AnimationMeta[animationInst.UpperAnimClipIdx];
        
        if (nowbone < upperMeta.BoneCount)
        {
            // Upper 애니메이션 샘플링
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
            
            // UPPER 애니메이션 전환 블렌드

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
            
            // ========== 5. UPPER/LOWER 레이어 블렌딩 (상하체 분리) ==========
            // 참고 코드: 상체 범위(0 ~ start)는 Upper만, 하체 범위(start ~ end)는 Lower만 사용
            // 경계 영역에서만 Feather로 블렌딩
            
            // 상하체 분리를 위한 본 범위 가중치 계산
            float upperBlendWeight = CalculateBlendWeight(
                nowbone,
                animationInst.UpperMaskStart,
                animationInst.UpperMaskEnd,
                featherRange
            );
            
            // 본의 BlendWeight와 결합
            float boneWeight = SkeletonBone[nowbone + boneidx].BlendWeight;
            float finalUpperWeight = saturate(animationInst.UpperLayerWeight) *
                                    saturate(boneWeight) *
                                    upperBlendWeight;
            
            // 상체 범위는 Upper 완전 적용, 경계만 블렌딩
            if (finalUpperWeight > 0.999f) // 완전히 Upper 범위
            {
                finalScale = upperScale;
                finalRotation = upperRotation;
                finalTranslation = upperTranslation;
            }
            else if (finalUpperWeight > 0.0001f) // 경계 영역 (Feather)
            {
                if (animationInst.UpperBlendMode == 1) // Additive
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
                else // Override 
                {
                    finalScale = lerp(lowerScale, upperScale, finalUpperWeight);
                    finalRotation = QuaternionSlerp(lowerRotation, upperRotation, finalUpperWeight);
                    finalTranslation = lerp(lowerTranslation, upperTranslation, finalUpperWeight);
                }
            }
            // else: finalUpperWeight가 0에 가까우면 Lower 그대로 사용 (하체 범위)
        }
    }
    finalRotation = normalize(finalRotation);
    finalScale = max(finalScale, float4(0.0001f, 0.0001f, 0.0001f, 1.0f));

    float4 quaternionzero = float4(0.f, 0.f, 0.f, 1.f);
    matrix matbone = MatrixAffineTransformation(finalScale, quaternionzero, finalRotation, finalTranslation);
    
    RFinalBone[animationInst.ReulstIndex + nowbone] = mul(SkeletonBone[nowbone + boneidx].Offset, matbone);
}