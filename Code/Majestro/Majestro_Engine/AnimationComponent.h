#pragma once
#include "Component.h"
#include "Skeleton.h"
#include "Animator.h"

class Material;
class StructuredBuffer;


struct AnimationInstance {
	uint32	SkeletonID{};		// 스켈레톤 핸들(Initialize에서 부여한 값)
	uint32	AnimClipID{};		// 현재 재생중인 애니메이션 클립 인덱스
	uint32	CurrentFrame{};		// 현재 프레임
	uint32	NextFrame{};		// 다음 프레임
	
	float	Ratio{};			// 현재 프레임과 다음 프레임 사이의 보간 비율 (0	~1)
	uint32	BoneCount{};		// 본 개수
	uint32	ReulstIndex{};		// CS가 쓰고 VS가 읽는 팔레트 시작 오프셋
	uint32  EntityID{};

	uint32	BlendClipID{};		// 보간 대상 클립 인덱스
	uint32	BlendCurrentFrame{};
	uint32	BlendNextFrame{};
	float	BlendRatio{};

	float	BlendWeight{};		// 0~1 (0이면 현재 클립만 사용)
};								


struct AnimatorParams
{
	uint32 skeletonID;   // 스켈레톤 핸들(Initialize에서 Skeleton::SetSkeletonHandle로 지정한 값)
	uint32 clipA;        // 현재 재생 클립(2-way 블렌드라면 B 포함)
	uint32 clipB;
	uint32 flags;        // 루프/루트모션/애디티브 등

	float  timeA;        // seconds
	float  timeB;
	float  weightB;      // 0~1
	float  playbackRate; // 1.0 = normal

	uint32 outBaseIndex; // ★ CS가 쓰고 VS가 읽는 팔레트 시작 오프셋
};

class AnimationComponent : public Component<AnimationComponent>
{
public:
	AnimationComponent();
	AnimationComponent(vector<shared_ptr<Animator>>& AnimClips);

public:
	uint32							mAnimInstanceID = 0;

	vector<shared_ptr<Animator>>	mAnimClips;
	AnimationInstance				mAnimInstance{};
	uint32							mAnimClipIdx{};		// 현재 재생중인 애니메이션 클립 인덱스
	uint32							mBlendClipIdx{};	// 보간 대상 클립 인덱스

	float							mBlendUpdateTime = 0.f;
	float							mBlendTimer = 0.f;
	float							mBlendDuration = 0.2f;
	float							mBlendWeight = 0.f;

	float							mUpdateTime = 0.f;	// 애니메이션 재생 시간
	bool							mBoneFinalUpdated = false;
};

