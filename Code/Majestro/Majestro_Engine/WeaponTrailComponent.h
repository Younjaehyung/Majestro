#pragma once
#include <string>
#include "Component.h"
#include "Entity.h"

struct WeaponTrailSample
{	
	// WeaponTrail 이동추적
	// (mFollowOwnerMovement == true 일 때만 채워진다)

	Vec3 TipWorld{};
	Vec3 BaseWorld{};

	// 기준 엔티티(플레이어) 로컬 공간으로 변환해 저장한 좌표.
	// 렌더 시 현재 기준 월드행렬로 다시 변환하면 과거 잔상도 플레이어 이동을 따라가며 사라진다.
	Vec3 TipLocal{};
	Vec3 BaseLocal{};
	float Age = 0.0f;
};

// WeaponTrail 이동추적: 기준 프레임은 스케일을 제거한 강체 변환(회전 옵션 + 위치)만 사용
inline Matrix BuildWeaponTrailReferenceFrame(Matrix& world, bool followRotation)
{
	Vec3 scale;
	Quaternion rotation;
	Vec3 translation;
	world.Decompose(scale, rotation, translation);

	if (followRotation)
		return Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);

	// 위치만 따라가기
	return Matrix::CreateTranslation(translation);
}

enum class WeaponTrailSource : uint8
{
	Transform = 0,
	Socket = 1,
};

enum class WeaponTrailVisualStyle : uint8
{
	FlameRibbon = 0,
	SwordSlash = 1,
	HammerFlame = 3,
};

class WeaponTrailComponent : public Component<WeaponTrailComponent>
{
public:
	static constexpr size_t MAX_SAMPLES = 256;

public:
	Entity mSourceEntity{};
	Vec3 mTipLocalOffset = Vec3(-6.0f, 9.0f, 3.25f);
	Vec3 mBaseLocalOffset = Vec3(17.0f, 1.0f, 7.0f);

	WeaponTrailSource mSourceType = WeaponTrailSource::Transform;
	std::string mTipSocketName;
	std::string mBaseSocketName;

	
	bool mFollowOwnerMovement = false;	// WeaponTrail 이동추적
	
	
	
	Entity mReferenceEntity{};				// 이동추적 기준 프레임으로 쓸 엔티티 (비어 있으면 mSourceEntity를 사용)
											// Socket 모드에서는 mSourceEntity가 플레이어 루트이므로 보통 비워둬도 됨
											// Transform 모드에서 mSourceEntity가 무기 자신이면, 여기에 플레이어 루트를 따로 지정해야 함
	
	bool mFollowReferenceRotation = true;	// true면 위치+회전 모두 따라가고, false면 위치만 따라간다(회전은 월드 고정).

	bool mAutoActivateOnAttack = false;

	bool mUseAnimationWindow = false;
	bool mUseUpperAnimationWindow = true;
	bool mUseAnimationFrameWindow = false;
	bool mActive = false;
	bool mResetOnActivate = true;
	float mTrailStartTime = 0.0f;
	float mTrailEndTime = 0.0f;
	uint32 mTrailStartFrame = 0;
	uint32 mTrailEndFrame = 0;
	float mLifetime = 0.16f;
	float mSampleInterval = 0.005f;
	float mMinSegmentDistance = 2.0f;
	float mFallbackWidth = 45.0f;
	float mBaseAlpha = 0.9f;
	float mIntensity = 2.8f;

	Vec3 mCoreColor = Vec3(1.0f, 0.78f, 0.24f);
	Vec3 mEdgeColor = Vec3(1.0f, 0.24f, 0.04f);
	Vec3 mSubColor = Vec3(0.38f, 0.08f, 0.02f);

	std::wstring mTextureName;

	WeaponTrailVisualStyle mVisualStyle = WeaponTrailVisualStyle::FlameRibbon;
	uint32 mLayerCount = 3;

	uint32 mSmoothingSubdivisions = 0;
	float mWidthMultiplier = 1.35f;
	float mTailWidthScale = 0.35f;
	float mMidWidthScale = 1.25f;
	float mHeadWidthScale = 0.28f;
	float mLayerSpread = 0.34f;
	float mSlashCutStrength = 0.75f;
	float mSlashLineStrength = 0.65f;

	// SwordSlash(검기) 전용 색 강도:
	float mSlashEdgeBoost = 1.6f;	// 폭 방향 외곽(양 끝)에 실리는 EdgeColor 발광 배수(타오르는 띠 강도)
	float mSlashCoreBoost = 1.0f;	// 중심선(side≈1)으로 섞이는 CoreColor 틴트 배수(흰 코어 채색 강도)

	float mSlashTexScrollSpeed = 0.0f;	// 텍스처/절차적 결을 길이(uv.y) 방향으로 시간에 따라 흘려보내는 스크롤 속도. 
	float mUvTiling = 1.0f;				// 길이 방향 텍스처 타일링

	std::vector<WeaponTrailSample> mSamples;
	float mTimeSinceLastSample = 0.0f;
	bool mWasActive = false;
};
