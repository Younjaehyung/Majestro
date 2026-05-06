#pragma once
#include <deque>

#include "Component.h"
#include "Entity.h"

struct TrailSocketDesc
{
	string boneName;
	uint32 boneIndex = UINT32_MAX;
	Matrix localOffset = Matrix::Identity;
};

struct TrailSample
{
	Vec3 basePos = Vec3::Zero;
	Vec3 tipPos = Vec3::Zero;
	float age = 0.f;
	float distanceFromStart = 0.f;
};

class WeaponTrailComponent : public Component<WeaponTrailComponent>
{
public:
	// 수정: trail 컴포넌트가 다른 엔티티의 애니메이션을 참조할 수 있도록 source entity를 분리했다.
	// 현재 플레이어 본체에 붙이는 경우에는 비워 두면 자기 자신의 애니메이션을 사용한다.
	Entity mSourceEntity;

	TrailSocketDesc mBaseSocket;
	TrailSocketDesc mTipSocket;

	bool mActive = false;
	bool mAutoActivateFromPlayerState = true;
	bool mUseAttackWindow = true;
	float mAttackWindowStart = 0.12f;
	float mAttackWindowEnd = 0.72f;
	float mLifeTime = 0.12f;
	float mMinSampleDistance = 3.f;
	float mInterpolationStep = 5.f;
	uint32 mMaxSamples = 32;
	Vec4 mColor = Vec4(0.35f, 0.85f, 1.0f, 0.75f);

	// 수정: trail 모양을 코드 고정 색상이 아니라 Texture 리소스로 바꿀 수 있도록 리소스 키와 샘플링 옵션을 분리했다.
	// mTextureName은 ResourceManager에 등록된 Texture 키이며, 비어 있거나 찾지 못하면 기존 색상 ribbon만 렌더링한다.
	wstring mTextureName;
	int32 mTextureIndex = -1;
	float mTextureAlphaWeight = 1.f;
	bool mUseTextureColor = false;

	std::deque<TrailSample> mSamples;
	bool mHasPreviousSample = false;
	Vec3 mPreviousBasePos = Vec3::Zero;
	Vec3 mPreviousTipPos = Vec3::Zero;
};
