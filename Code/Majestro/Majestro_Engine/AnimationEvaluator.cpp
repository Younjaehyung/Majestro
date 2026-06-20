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
	Matrix* outFinalBones,
	const AimParams* aim)
{
	// spine 체인 가중치 분배 (합 = 1.0)
	auto getAimWeight = [&](uint32 boneIdx) -> float
	{
		if (!aim) return 0.f;
		if (boneIdx == aim->Spine1Idx) return aim->Spine1Weight;
		if (boneIdx == aim->Spine2Idx) return aim->Spine2Weight;
		if (boneIdx == aim->Spine3Idx) return aim->Spine3Weight;
		if (boneIdx == aim->NeckIdx)   return aim->NeckWeight;
		return 0.f;
	};
	const AnimationClipMeta& clipMeta = metaBuffer[inst.AnimClipID];
	const uint32 boneIdxBase = clipMeta.BoneStart;
	const uint32 boneCount = clipMeta.BoneCount;

	const float featherRange = 4.0f;

	

	constexpr bool kUseMeshSpaceUpperRotation = true;	// 상체 레이어 회전을 모델(메쉬)공간에서 블렌드할지 여부.
	//  true  : 하체(골반) 회전이 상체로 전파되지 않음 
	//  false : 기존 로컬공간 회전 블렌드(하체 뒤틀림이 상체로 상속됨)

	if (kUseMeshSpaceUpperRotation)
	{
		baseMeshRot.clear();
		upperMeshRot.clear();
		blendMeshRot.clear();
		effUpperW.clear();


		baseMeshRot.resize(boneCount);
		upperMeshRot.resize(boneCount);
		blendMeshRot.resize(boneCount);
		effUpperW.resize(boneCount);
	}

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

		// 하체 레이어 로컬 회전(이후 메쉬 누적의 기준)
		const Vec4 baseLocalQ = lowerRotation;

		// 상체 Upper 레이어 샘플 + 가중치
		Vec4 us = lowerScale;
		Vec4 ur = lowerRotation;
		Vec4 ut = lowerTranslation;
		float finalUpperW = 0.0f;
		bool  hasUpper = false;

		if (inst.UpperLayerWeight > 0.0001f && inst.UpperAnimClipIdx != inst.AnimClipID)
		{
			if (inst.UpperAnimClipIdx < metaCount)
			{
				const AnimationClipMeta& upperMeta = metaBuffer[inst.UpperAnimClipIdx];
				if (nowbone < upperMeta.BoneCount)
				{
					hasUpper = true;
					SampleAnimation(
						nowbone, upperMeta.NumFrame,
						inst.UpperCurrentFrame, inst.UpperNextFrame, inst.UpperRatio,
						upperMeta.AnimOffset, clipBuffer,
						us, ur, ut);

					// 상체 클립 크로스페이드
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
					finalUpperW = Saturate(inst.UpperLayerWeight) * Saturate(boneW) * upperBlendW;
				}
			}
		}

		const Vec4 upperLocalQ = hasUpper ? ur : baseLocalQ;

		// 메쉬공간 모드: 상체 가중치를 계층으로 전파
		if (kUseMeshSpaceUpperRotation)
		{
			const int32 wpIdx = boneBuffer[nowbone + boneIdxBase].parentIdx;
			float eff = finalUpperW;
			if (wpIdx >= 0 && static_cast<uint32>(wpIdx) < boneCount)
				eff = max(eff, effUpperW[wpIdx]);
			effUpperW[nowbone] = eff;
			finalUpperW = eff;
		}

		Vec4 finalScale = lowerScale;
		Vec4 finalRotation = lowerRotation;
		Vec4 finalTranslation = lowerTranslation;

		const bool additive = (inst.UpperBlendMode == 1);

		// Scale / Translation 블렌드
		if (additive && finalUpperW > 0.0001f)
		{
			const Vec4 idS(1, 1, 1, 1);
			const Vec4 eps(0.0001f, 0.0001f, 0.0001f, 0.0001f);

			Vec4 deltaS = DivCompV4(us, MaxV4(lowerScale, eps));
			Vec4 addS = LerpV4(idS, deltaS, finalUpperW);
			finalScale = MulCompV4(lowerScale, addS);

			Vec4 deltaT = ut - lowerTranslation;
			finalTranslation = lowerTranslation + deltaT * finalUpperW;
		}
		else
		{
			finalScale = LerpV4(lowerScale, us, finalUpperW);
			finalTranslation = LerpV4(lowerTranslation, ut, finalUpperW);
		}

		// 회전 블렌드
		if (kUseMeshSpaceUpperRotation)
		{
			const int32 pIdx = boneBuffer[nowbone + boneIdxBase].parentIdx;
			const bool hasParent = (pIdx >= 0 && static_cast<uint32>(pIdx) < boneCount);

			// 두 레이어의 모델공간 회전 누적 (parent < nowbone 보장)
			const Vec4 baseMesh = hasParent ? HlslQuatMul(baseMeshRot[pIdx], baseLocalQ) : baseLocalQ;
			Vec4 upperMesh = hasParent ? HlslQuatMul(upperMeshRot[pIdx], upperLocalQ) : upperLocalQ;
			baseMeshRot[nowbone] = baseMesh;
			upperMeshRot[nowbone] = upperMesh;

			if (additive && finalUpperW > 0.0001f)
			{
				// Additive
				const Vec4 idQ(0, 0, 0, 1);
				Vec4 deltaR = HlslQuatMul(ur, HlslQuatConj(lowerRotation));
				Vec4 addR = HlslQuatSlerp(idQ, deltaR, finalUpperW);
				finalRotation = HlslQuatMul(lowerRotation, addR);

				// 자식 체인 일관성: 최종 로컬 회전(aim 적용 전)을 누적
				blendMeshRot[nowbone] = hasParent
					? HlslQuatMul(blendMeshRot[pIdx], finalRotation)
					: finalRotation;
			}
			else
			{
				// Override
				float dot = baseMesh.x * upperMesh.x + baseMesh.y * upperMesh.y +
					baseMesh.z * upperMesh.z + baseMesh.w * upperMesh.w;
				if (dot < 0.f)
					upperMesh = Vec4(-upperMesh.x, -upperMesh.y, -upperMesh.z, -upperMesh.w);

				const Vec4 Rm = HlslQuatSlerp(baseMesh, upperMesh, Saturate(finalUpperW));
				blendMeshRot[nowbone] = Rm; // aim 적용 전 모델공간 회전(자식 변환 기준)

				// 모델공간 회전을 (블렌드된)부모 기준 로컬 회전으로 환산.
				finalRotation = hasParent
					? HlslQuatMul(HlslQuatConj(blendMeshRot[pIdx]), Rm)
					: Rm;
			}
		}
		else
		{	// 상체 전파
			if (additive && finalUpperW > 0.0001f)
			{
				const Vec4 idQ(0, 0, 0, 1);
				Vec4 deltaR = HlslQuatMul(ur, HlslQuatConj(lowerRotation));
				Vec4 addR = HlslQuatSlerp(idQ, deltaR, finalUpperW);
				finalRotation = HlslQuatMul(lowerRotation, addR);
			}
			else
			{
				finalRotation = HlslQuatSlerp(lowerRotation, ur, finalUpperW);
			}
		}

		// ---- Aim / Hit 오프셋 (기존과 동일, 로컬 후처리) ----
		const float aimW = getAimWeight(nowbone);
		if (aimW > 0.f && aim)
		{
			// AimPitch/AimYaw(조준 입력) + HitPitch/HitYaw(피격 움찔) 합산.
			const float pitch = (aim->AimPitch + aim->HitPitch) * aimW;
			const float yaw = (aim->AimYaw + aim->HitYaw) * aimW;
			if (fabsf(pitch) > 1e-5f || fabsf(yaw) > 1e-5f)
			{
				// 본 로컬 축: Y = pitch(앞뒤), Z = yaw(좌우).
				const Vec4 pitchQ = QuatFromAxisAngle(Vec3::UnitY, -pitch);
				const Vec4 yawQ = QuatFromAxisAngle(Vec3::UnitZ, yaw);
				const Vec4 aimQ = HlslQuatMul(pitchQ, yawQ);
				finalRotation = HlslQuatMul(finalRotation, aimQ);
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
		Matrix localBone = sMat * rMat * tMat;

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
