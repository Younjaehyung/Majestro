#pragma once
#include "Component.h"
#include "Entity.h"

struct DashSpeedLineSample
{
	Vec3 CenterWorld{};
	Vec3 DirectionWorld = Vec3::Forward;
	Vec3 RightWorld = Vec3::Right;
	float Age = 0.0f;
};

class DashSpeedLineComponent : public Component<DashSpeedLineComponent>
{
public:
	static constexpr size_t MAX_SAMPLES = 96;

public:
	Entity mSourceEntity{};

	bool mAutoActivateOnDash = true;
	bool mActive = false;
	bool mWasActive = false;

	// 연기/베이퍼
	float mLifetime = 0.45f;        
	float mSampleInterval = 0.01f;
	float mMinSegmentDistance = 6.0f;
	float mHeightOffset = 75.0f;
	float mBackOffset = 50.0f;         

	uint32 mLineCount = 5;
	float mLineWidth = 40.0f;          
	float mLineSpread = 70.0f;         
	float mBaseAlpha = 0.35f;          
	float mIntensity = 1.1f;
	float mUvTiling = 1.5f;            // 길이 방향 연기 결 디테일
	float mBreakStrength = 0.7f;       // 흩어지는 연기 외곽
	float mLineStrength = 0.5f;        // 밀도 코어

	Vec3 mCoreColor = Vec3(1.0f, 1.0f, 1.05f);   // 본체 밝은 차가운 흰색
	Vec3 mEdgeColor = Vec3(0.7f, 0.78f, 0.9f);   // 가장자리 연한 회청색
	Vec3 mSubColor = Vec3(0.25f, 0.3f, 0.4f);    // 외곽 어두운 연기색

	std::wstring mTextureName;

	std::vector<DashSpeedLineSample> mSamples;
	float mTimeSinceLastSample = 0.0f;

	
	bool mHasLastSourcePosition = false;
	Vec3 mLastSourcePosition{};
};
