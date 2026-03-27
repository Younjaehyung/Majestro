#include "params.hlsl"
#include "utils.hlsl"
#include "math.hlsl"

// [추가] 레이어 내부 블렌딩 결과를 담기 위한 구조체
struct LayerPose
{
    float4 Scale;
    float4 Rotation;
    float4 Translation;
};

// [추가] 하나의 애니메이션 클립을 샘플링
LayerPose SamplePose(
    uint boneIndex,
    uint clipIdx,
    uint currentFrame,
    uint nextFrame,
    float ratio)
{
    LayerPose pose = (LayerPose) 0;

    ANIMATIONMETA meta = AnimationMeta[clipIdx];

    if (boneIndex < meta.BoneCount)
    {
        SampleAnimation(
            boneIndex,
            meta.NumFrame,
            currentFrame,
            nextFrame,
            ratio,
            meta.AnimOffset,
            pose.Scale,
            pose.Rotation,
            pose.Translation
        );
    }
    else
    {
        // [추가] BoneCount 밖이면 identity에 가까운 기본값
        pose.Scale = float4(1.f, 1.f, 1.f, 1.f);
        pose.Rotation = float4(0.f, 0.f, 0.f, 1.f);
        pose.Translation = float4(0.f, 0.f, 0.f, 0.f);
    }

    return pose;
}

// [추가] 같은 레이어 내부의 current ↔ next 블렌딩
LayerPose BlendLayerPose(
    LayerPose currentPose,
    LayerPose nextPose,
    float blendWeight,
    uint blendMode)
{
    LayerPose result = currentPose;

    float w = saturate(blendWeight);

    if (w <= 0.0001f)
        return result;

    if (blendMode == 1)
    {
        const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
        const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

        // [수정] additive 레이어 블렌드
        float4 additiveScale = lerp(identityScale, nextPose.Scale, w);
        result.Scale.xyz *= additiveScale.xyz;

        float4 additiveRotation = QuaternionSlerp(identityRotation, nextPose.Rotation, w);
        result.Rotation = QuaternionMultiply(result.Rotation, additiveRotation);

        result.Translation.xyz += nextPose.Translation.xyz * w;
    }
    else
    {
        // [수정] 일반 cross-fade
        result.Scale = lerp(currentPose.Scale, nextPose.Scale, w);
        result.Rotation = QuaternionSlerp(currentPose.Rotation, nextPose.Rotation, w);
        result.Translation = lerp(currentPose.Translation, nextPose.Translation, w);
    }

    return result;
}

// [추가] LowerPose ↔ UpperPose 최종 합성
LayerPose ComposeMaskedPose(
    LayerPose lowerPose,
    LayerPose upperPose,
    float finalUpperWeight,
    uint upperBlendMode)
{
    LayerPose result = lowerPose;
    float w = saturate(finalUpperWeight);

    if (w <= 0.0001f)
        return result;

    if (w > 0.999f)
        return upperPose;

    if (upperBlendMode == 1)
    {
        const float4 identityRotation = float4(0.f, 0.f, 0.f, 1.f);
        const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

        float4 deltaRotation = QuaternionMultiply(upperPose.Rotation, QuaternionConjugate(lowerPose.Rotation));
        float4 additiveRotation = QuaternionSlerp(identityRotation, deltaRotation, w);
        result.Rotation = QuaternionMultiply(lowerPose.Rotation, additiveRotation);

        float4 deltaScale = upperPose.Scale / max(lowerPose.Scale, float4(0.0001f, 0.0001f, 0.0001f, 0.0001f));
        float4 additiveScale = lerp(identityScale, deltaScale, w);
        result.Scale = lowerPose.Scale * additiveScale;

        float4 deltaTranslation = upperPose.Translation - lowerPose.Translation;
        result.Translation = lowerPose.Translation + deltaTranslation * w;
    }
    else
    {
        result.Scale = lerp(lowerPose.Scale, upperPose.Scale, w);
        result.Rotation = QuaternionSlerp(lowerPose.Rotation, upperPose.Rotation, w);
        result.Translation = lerp(lowerPose.Translation, upperPose.Translation, w);
    }

    return result;
}

[numthreads(1, 4, 1)]
void CS_Main(int3 threadIdx : SV_DispatchThreadID)
{
    uint rel = threadIdx.y;
    uint inst = GlobalParams.BaseInstanceID + rel;

    if (rel >= GlobalParams.etc)
        return;

    ANIMINSTANCE animationInst = AnimInstance[inst];
    ANIMATIONMETA skeletonMeta = AnimationMeta[animationInst.AnimClipIdx];

    uint boneidx = skeletonMeta.BoneStart;
    uint bonecount = skeletonMeta.BoneCount;

    const float featherRange = 4.0f;

    // 1) 로컬 본 포즈 계산 + 부모 누적(모델 공간) 저장
    for (uint nowbone = 0; nowbone < bonecount; ++nowbone)
    {
        // ============================================================
        // [수정] Lower 레이어를 "독립적으로" 계산
        // LowerCurrent ↔ LowerNext
        // ============================================================
        LayerPose lowerCurrentPose = SamplePose(
            nowbone,
            animationInst.AnimClipIdx,
            animationInst.CurrentFrame,
            animationInst.NextFrame,
            animationInst.Ratio
        );

        LayerPose lowerNextPose = lowerCurrentPose;
        if (animationInst.BlendClipIdx != animationInst.AnimClipIdx)
        {
            lowerNextPose = SamplePose(
                nowbone,
                animationInst.BlendClipIdx,
                animationInst.BlendCurrentFrame,
                animationInst.BlendNextFrame,
                animationInst.BlendRatio
            );
        }

        LayerPose lowerPose = BlendLayerPose(
            lowerCurrentPose,
            lowerNextPose,
            animationInst.BlendWeight,
            animationInst.BlendMode
        );

        // ============================================================
        // [수정] Upper 레이어를 "독립적으로" 계산
        // UpperCurrent ↔ UpperNext
        // 기존의 "if (UpperAnimClipIdx != AnimClipIdx)" 조건 제거
        // ============================================================
        LayerPose upperCurrentPose = SamplePose(
            nowbone,
            animationInst.UpperAnimClipIdx,
            animationInst.UpperCurrentFrame,
            animationInst.UpperNextFrame,
            animationInst.UpperRatio
        );

        LayerPose upperNextPose = upperCurrentPose;
        if (animationInst.UpperBlendClipIdx != animationInst.UpperAnimClipIdx)
        {
            upperNextPose = SamplePose(
                nowbone,
                animationInst.UpperBlendClipIdx,
                animationInst.UpperBlendCurrentFrame,
                animationInst.UpperBlendNextFrame,
                animationInst.UpperBlendRatio
            );
        }

        LayerPose upperPose = BlendLayerPose(
            upperCurrentPose,
            upperNextPose,
            animationInst.UpperBlendWeight,
            animationInst.UpperBlendMode
        );

        // ============================================================
        // [유지 + 정리] 본 마스크 기반 최종 상하체 합성 가중치 계산
        // ============================================================
        //float boneWeight = SkeletonBone[nowbone + boneidx].BlendWeight;

        //float maskWeight = 0.0f;
        //if (boneWeight > 0.0001f && nowbone >= animationInst.UpperMaskStart)
        //{
        //    maskWeight = CalculateBlendWeight(
        //        nowbone,
        //        animationInst.UpperMaskStart,
        //        animationInst.UpperMaskEnd,
        //        featherRange
        //    );
        //}

        //float finalUpperWeight =
        //    saturate(animationInst.UpperLayerWeight) *
        //    saturate(maskWeight) *
        //    saturate(boneWeight);

        // ============================================================
        // 최종 가중치: UpperLayerWeight(점진적 보간) × 본별 BlendWeight
        // ============================================================
        float boneBlendWeight  = SkeletonBone[nowbone + boneidx].BlendWeight;
        float finalUpperWeight = saturate(animationInst.UpperLayerWeight) * boneBlendWeight;

        // ============================================================
        // 상하체 합성 (하이브리드 방식)
        //
        // ▸ 척추 전환 영역 (0 < boneBlendWeight < 1, RefClipIdx 설정 시)
        //     → Additive: delta = upper - ref, final = lower + delta * w
        //       부모 좌표계 오염 없이 조준 방향을 보정한다.
        //
        // ▸ 완전 상체 본 (boneBlendWeight ≥ 1, 팔/어깨/목/머리)
        //     → Override: upper 포즈를 그대로 사용
        //       Additive로 걷기 팔 동작 + 조준 델타가 합산되면 팔이 꼬임.
        //
        // ▸ 완전 하체 본 (boneBlendWeight = 0) / RefClipIdx 미설정
        //     → Override 폴백
        // ============================================================
        LayerPose finalPose;

        bool isTransitionBone = (boneBlendWeight > 0.0001f && boneBlendWeight < 0.9999f);

        if (animationInst.RefClipIdx != 0xFFFFFFFFu && isTransitionBone)
        {
            // ── Additive 경로: 척추 전환 영역만 적용 ──────────────
            LayerPose refPose = SamplePose(
                nowbone,
                animationInst.RefClipIdx,
                animationInst.RefCurrentFrame,
                animationInst.RefNextFrame,
                animationInst.RefRatio
            );

            const float4 identityRot   = float4(0.f, 0.f, 0.f, 1.f);
            const float4 identityScale = float4(1.f, 1.f, 1.f, 1.f);

            // delta = upper - ref  (쿼터니언: conjugate(ref) * upper)
            float4 deltaRot   = QuaternionMultiply(QuaternionConjugate(refPose.Rotation), upperPose.Rotation);
            float4 deltaTrans = upperPose.Translation - refPose.Translation;
            float4 deltaScale = upperPose.Scale / max(refPose.Scale,
                                    float4(0.0001f, 0.0001f, 0.0001f, 0.0001f));

            // delta에 가중치 적용 (UpperLayerWeight × boneBlendWeight)
            float4 weightedDeltaRot   = QuaternionSlerp(identityRot,   deltaRot,   finalUpperWeight);
            float4 weightedDeltaScale = lerp(identityScale, deltaScale, finalUpperWeight);

            // final = lower + delta * w
            finalPose.Rotation    = QuaternionMultiply(lowerPose.Rotation, weightedDeltaRot);
            finalPose.Scale       = lowerPose.Scale * weightedDeltaScale;
            finalPose.Translation = lowerPose.Translation + deltaTrans * finalUpperWeight;
        }
        else
        {
            // ── Override 경로: 팔/머리(weight=1), 하체(weight=0), RefClipIdx 미설정 ──
            finalPose = ComposeMaskedPose(
                lowerPose,
                upperPose,
                finalUpperWeight,
                animationInst.UpperBlendMode
            );
        }
        //LayerPose finalPose = lowerPose;
        //if (animationInst.UpperMaskStart < nowbone && animationInst.UpperMaskEnd >= nowbone)
        //{
        //    finalPose = upperPose;
        //}
        
        finalPose.Rotation = normalize(finalPose.Rotation);
        finalPose.Scale = max(finalPose.Scale, float4(0.0001f, 0.0001f, 0.0001f, 1.0f));

        float4 quaternionzero = float4(0.f, 0.f, 0.f, 1.f);
        matrix localBone = MatrixAffineTransformation(
            finalPose.Scale,
            quaternionzero,
            finalPose.Rotation,
            finalPose.Translation
        );

        int parentIdx = SkeletonBone[nowbone + boneidx].ParentIdx;
        matrix modelBone = localBone;

        if (parentIdx >= 0 && parentIdx < int(bonecount))
        {
            matrix parentModel = RFinalBone[animationInst.ReulstIndex + (uint) parentIdx];
            modelBone = mul(localBone, parentModel);
        }

        RFinalBone[animationInst.ReulstIndex + nowbone] = modelBone;
    }

    
    
    // 2) Offset 적용
    for (uint nowbone = 0; nowbone < bonecount; ++nowbone)
    {
        matrix modelBone = RFinalBone[animationInst.ReulstIndex + nowbone];
        RFinalBone[animationInst.ReulstIndex + nowbone] =
            mul(SkeletonBone[boneidx + nowbone].Offset, modelBone);
    }
}