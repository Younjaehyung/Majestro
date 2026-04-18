#include "pch.h"
#include "AnimationEvaluator.h"
#include "AnimationComponent.h"
#include "Animator.h"
#include "Skeleton.h"
#include "MathUtils.h"

void AnimationEvaluator::Evaluate(
	const AnimationInstance& inst,
	const KeyFrameInfo* clipBuffer,
	const AnimationClipMeta* metaBuffer,
	uint32 metaCount,
	const SkeletonBoneParams* boneBuffer,
	Matrix* outModelBones,
	Matrix* outFinalBones)
{
	const AnimationClipMeta& clipMeta = metaBuffer[inst.AnimClipID];
	const uint32 boneIdxBase = clipMeta.BoneStart;
	const uint32 boneCount = clipMeta.BoneCount;

	const float featherRange = 4.0f;

	// 로컬 본 포즈 + 부모 누적(model space) 계산
	for (uint32 nowbone = 0; nowbone < boneCount; ++nowbone)
	{
		Vec4 lowerScale, lowerRotation, lowerTranslation;
		SampleAnimation(
			nowbone, clipMeta.NumFrame,
			inst.CurrentFrame, inst.NextFrame, inst.Ratio,
			clipMeta.AnimOffset, clipBuffer,
			lowerScale, lowerRotation, lowerTranslation);

		// 하체 Base 레이어 블렌드
		if (inst.BlendWeight > 0.0001f && inst.BlendClipID != inst.AnimClipID)
		{
			if (inst.BlendClipID < metaCount)
			{
				const AnimationClipMeta& blendMeta = metaBuffer[inst.BlendClipID];
				if (nowbone < blendMeta.BoneCount)
				{
					Vec4 bs, br, bt;
					SampleAnimation(
						nowbone, blendMeta.NumFrame,
						inst.BlendCurrentFrame, inst.BlendNextFrame, inst.BlendRatio,
						blendMeta.AnimOffset, clipBuffer,
						bs, br, bt);

					const float w = Saturate(inst.BlendWeight);

					if (inst.BlendMode == 1)
					{
						const Vec4 idQ(0, 0, 0, 1);
						const Vec4 idS(1, 1, 1, 1);

						Vec4 addS = LerpV4(idS, bs, w);
						lowerScale.x *= addS.x; lowerScale.y *= addS.y; lowerScale.z *= addS.z;

						Vec4 addR = HlslQuatSlerp(idQ, br, w);
						lowerRotation = HlslQuatMul(lowerRotation, addR);

						lowerTranslation.x += bt.x * w;
						lowerTranslation.y += bt.y * w;
						lowerTranslation.z += bt.z * w;
					}
					else
					{
						lowerScale = LerpV4(lowerScale, bs, w);
						lowerRotation = HlslQuatSlerp(lowerRotation, br, w);
						lowerTranslation = LerpV4(lowerTranslation, bt, w);
					}
				}
			}
		}

		Vec4 finalScale = lowerScale;
		Vec4 finalRotation = lowerRotation;
		Vec4 finalTranslation = lowerTranslation;

		// 상체 Upper 레이어
		if (inst.UpperLayerWeight > 0.0001f && inst.UpperAnimClipIdx != inst.AnimClipID)
		{
			if (inst.UpperAnimClipIdx < metaCount)
			{
				const AnimationClipMeta& upperMeta = metaBuffer[inst.UpperAnimClipIdx];
				if (nowbone < upperMeta.BoneCount)
				{
					Vec4 us, ur, ut;
					SampleAnimation(
						nowbone, upperMeta.NumFrame,
						inst.UpperCurrentFrame, inst.UpperNextFrame, inst.UpperRatio,
						upperMeta.AnimOffset, clipBuffer,
						us, ur, ut);

					if (inst.UpperBlendWeight > 0.0001f &&
						inst.UpperBlendClipIdx != inst.UpperAnimClipIdx &&
						inst.UpperBlendClipIdx < metaCount)
					{
						const AnimationClipMeta& upperBlendMeta = metaBuffer[inst.UpperBlendClipIdx];
						if (nowbone < upperBlendMeta.BoneCount)
						{
							Vec4 ubs, ubr, ubt;
							SampleAnimation(
								nowbone, upperBlendMeta.NumFrame,
								inst.UpperBlendCurrentFrame, inst.UpperBlendNextFrame, inst.UpperBlendRatio,
								upperBlendMeta.AnimOffset, clipBuffer,
								ubs, ubr, ubt);

							const float w = Saturate(inst.UpperBlendWeight);
							us = LerpV4(us, ubs, w);
							ur = HlslQuatSlerp(ur, ubr, w);
							ut = LerpV4(ut, ubt, w);
						}
					}

					float upperBlendW = 0.0f;
					if (nowbone >= inst.UpperMaskStart)
					{
						upperBlendW = CalculateBlendWeight(
							nowbone, inst.UpperMaskStart, inst.UpperMaskEnd, featherRange);
					}

					const float boneW = boneBuffer[nowbone + boneIdxBase].blendWeight;
					const float finalUpperW = Saturate(inst.UpperLayerWeight) * Saturate(boneW) * upperBlendW;

					if (finalUpperW > 0.999f)
					{
						finalScale = us;
						finalRotation = ur;
						finalTranslation = ut;
					}
					else if (finalUpperW > 0.0001f)
					{
						if (inst.UpperBlendMode == 1)
						{
							const Vec4 idQ(0, 0, 0, 1);
							const Vec4 idS(1, 1, 1, 1);
							const Vec4 eps(0.0001f, 0.0001f, 0.0001f, 0.0001f);

							Vec4 deltaR = HlslQuatMul(ur, HlslQuatConj(lowerRotation));
							Vec4 addR = HlslQuatSlerp(idQ, deltaR, finalUpperW);
							finalRotation = HlslQuatMul(lowerRotation, addR);

							Vec4 deltaS = DivCompV4(us, MaxV4(lowerScale, eps));
							Vec4 addS = LerpV4(idS, deltaS, finalUpperW);
							finalScale = MulCompV4(lowerScale, addS);

							Vec4 deltaT = ut - lowerTranslation;
							finalTranslation = lowerTranslation + deltaT * finalUpperW;
						}
						else
						{
							finalScale = LerpV4(lowerScale, us, finalUpperW);
							finalRotation = HlslQuatSlerp(lowerRotation, ur, finalUpperW);
							finalTranslation = LerpV4(lowerTranslation, ut, finalUpperW);
						}
					}
				}
			}
		}

		// 정규화 / 클램프
		{
			const float n2 = finalRotation.x * finalRotation.x + finalRotation.y * finalRotation.y +
				finalRotation.z * finalRotation.z + finalRotation.w * finalRotation.w;
			if (n2 > 1e-12f)
			{
				const float inv = 1.f / std::sqrt(n2);
				finalRotation.x *= inv; finalRotation.y *= inv; finalRotation.z *= inv; finalRotation.w *= inv;
			}
		}
		finalScale.x = max(finalScale.x, 0.0001f);
		finalScale.y = max(finalScale.y, 0.0001f);
		finalScale.z = max(finalScale.z, 0.0001f);

		// row-major math SRT (vec * (S * R * T))
		const Matrix sMat = Matrix::CreateScale(finalScale.x, finalScale.y, finalScale.z);
		const Matrix rMat = Matrix::CreateFromQuaternion(Quaternion(finalRotation.x, finalRotation.y, finalRotation.z, finalRotation.w));
		const Matrix tMat = Matrix::CreateTranslation(finalTranslation.x, finalTranslation.y, finalTranslation.z);
		const Matrix localBone = sMat * rMat * tMat;

		const int32 parentIdx = boneBuffer[nowbone + boneIdxBase].parentIdx;
		Matrix modelBone = localBone;
		if (parentIdx >= 0 && static_cast<uint32>(parentIdx) < boneCount)
		{
			modelBone = localBone * outModelBones[parentIdx];
		}

		outModelBones[nowbone] = modelBone;
	}

	//  최종 스키닝 행렬: matOffset.Transpose() * modelBone (row-major math)
	for (uint32 nowbone = 0; nowbone < boneCount; ++nowbone)
	{
		const Matrix offsetT = boneBuffer[nowbone + boneIdxBase].matOffset.Transpose();
		outFinalBones[nowbone] = offsetT * outModelBones[nowbone];
	}
}

float AnimationEvaluator::CalculateBlendWeight(uint32 boneIndex, uint32 rangeStart, uint32 rangeEnd, float featherRange)
{
	if (rangeEnd <= rangeStart)
		return 0.0f;

	const float bf = static_cast<float>(boneIndex);
	const float s = static_cast<float>(rangeStart);
	const float e = static_cast<float>(rangeEnd);

	if (bf < s - featherRange || bf > e + featherRange)
		return 0.0f;

	const float rise = std::clamp((bf - (s - featherRange)) / featherRange, 0.f, 1.f);
	const float fall = std::clamp(((e + featherRange) - bf) / featherRange, 0.f, 1.f);
	return rise * fall;
}

void AnimationEvaluator::SampleAnimation(
	uint32 boneIndex,
	uint32 frameCount,
	uint32 currentFrame,
	uint32 nextFrame,
	float ratio,
	uint32 animOffset,
	const KeyFrameInfo* clipBuffer,
	Vec4& outScale, Vec4& outRotation, Vec4& outTranslation)
{
	const uint32 idx = boneIndex * frameCount + currentFrame + animOffset;
	const uint32 nextIdx = boneIndex * frameCount + nextFrame + animOffset;

	const KeyFrameInfo& a = clipBuffer[idx];
	const KeyFrameInfo& b = clipBuffer[nextIdx];

	outScale = LerpV4(a.scale, b.scale, ratio);
	outRotation = HlslQuatSlerp(a.rotation, b.rotation, ratio);
	outTranslation = LerpV4(a.translate, b.translate, ratio);
}
