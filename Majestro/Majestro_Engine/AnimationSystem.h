#pragma once
#include "World.h"
#include "System.h"

struct Bucket { uint32 start; uint32 count; uint32 bones; };

struct CSBatchCB {
	uint32 InstanceStart;      // mAnimationPass 전역 시작 인덱스
	uint32 InstanceCount;      // 이 배치에서 처리할 인스턴스 수
};

class AnimationSystem : public System
{
public:
	AnimationSystem(World* world);
	void Initialize();

	void Update(float);

private:	// COMPUTE 애니메이션 시스템

	void ClearVector();

	void AnimationPush(float);
	void AnimationCompute();
	void AnimationBlend(float);

	
	void AnimationDispatch();


private:	// 애니메이션 시스템
	shared_ptr<class Shader> mAnimationShader;
	const uint32_t TX = 64; // numthreads X
	const uint32_t TY = 4; // numthreads Y

private:
	vector<struct KeyFrameInfo> mAniKeyFrame;
	vector<struct AnimationClipMeta> mAniClipMeta;
	vector<Matrix> mBoneData;


	vector<struct AnimationInstance> mAnimationPass;
	vector<Bucket> mAnimationBuckets;
	
};
