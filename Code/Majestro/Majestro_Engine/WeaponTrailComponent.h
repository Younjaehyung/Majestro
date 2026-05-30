#pragma once
#include <string>
#include "Component.h"
#include "Entity.h"

struct WeaponTrailSample
{
	Vec3 TipWorld{};
	Vec3 BaseWorld{};
	float Age = 0.0f;
};

enum class WeaponTrailSource : uint8
{
	Transform = 0, // 기존 방식 — 소스 엔티티 월드행렬 + local offset
	Socket = 1,	   // 캐릭터 본을 따라가는 SocketComponent의 소켓 월드행렬에서 tip/base를 읽는다
};

class WeaponTrailComponent : public Component<WeaponTrailComponent>
{
public:
	static constexpr size_t MAX_SAMPLES = 64;

public:
	Entity mSourceEntity{};
	Vec3 mTipLocalOffset = Vec3(0.0f, 0.0f, 120.0f);
	Vec3 mBaseLocalOffset = Vec3(0.0f, 0.0f, 70.0f);

	WeaponTrailSource mSourceType = WeaponTrailSource::Transform;
	std::string mTipSocketName;   // Socket 모드: mSourceEntity의 SocketComponent의 tip 소켓 이름
	std::string mBaseSocketName;  // Socket 모드: mSourceEntity의 SocketComponent의 base 소켓 이름



	bool mAutoActivateOnAttack = false;	  	// true : WeaponTrailSystem이 mSourceEntity의 MainPlayerComponent 
											// 공격 상태(Attack1/2, Skill1/2, Special)에 따라 mActive를 매 프레임 자동 토글
											// false : 수동 토글

	bool mActive = false;
	bool mResetOnActivate = true;
	float mLifetime = 0.22f;
	float mSampleInterval = 0.008f;
	float mMinSegmentDistance = 4.0f;
	float mFallbackWidth = 45.0f;
	float mBaseAlpha = 1.0f;
	float mIntensity = 2.0f;

	Vec3 mCoreColor = Vec3(1.0f, 0.72f, 0.22f);
	Vec3 mEdgeColor = Vec3(1.0f, 0.18f, 0.03f);

	std::wstring mTextureName;	// WeaponTrail 텍스처:

	std::vector<WeaponTrailSample> mSamples;
	float mTimeSinceLastSample = 0.0f;
	bool mWasActive = false;
};
