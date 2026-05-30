#pragma once
#include "World.h"
#include "System.h"
#include "Animator.h"
#include "Skeleton.h"
#include "AnimationComponent.h"

class CpuAnimationSystem : public System
{
public:
	CpuAnimationSystem(World* world);
	void Initialize() override;
	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;


	// 소켓 어태치용 모델 공간 본 행렬 조회. boneIndex는 스켈레톤 로컬 본 인덱스(Skeleton::TryFindBoneIndex 결과) 사용
	bool GetModelBoneMatrix(Entity entity, uint32 boneIndex, Matrix& outMatrix) const;

private:
	void ClearVector();
	void AnimationPush(float deltaTime);
	void EvaluateAndUpload();
	void AnimationBlend(const shared_ptr<Animator>& animClip, float updateTime,
		uint32& currentFrame, uint32& nextFrame, float& ratio);

private:
	vector<KeyFrameInfo>       mAniKeyFrame;
	vector<AnimationClipMeta>  mAniClipMeta;
	vector<SkeletonBoneParams> mBoneData;

	vector<AnimationInstance>  mAnimationPass;
	// mAnimationPass와 동일 인덱스 — AimOffset (CPU 전용)
	vector<AimParams>          mAimPass;

	// 최종 본 행렬 (GPU 업로드 직전 transpose 적용)
	vector<Matrix>             mFinalBoneUpload;
	// 인스턴스별 모델 공간 본(부모 누적) — VFX/소켓 어태치용 캐시
	vector<Matrix>             mScratchModelBones;


	// 엔티티별 모델 공간 본 행렬(offset 미적용, row-major) 캐시.
	std::unordered_map<EntityID, std::vector<Matrix>> mEntityModelBones;
};
