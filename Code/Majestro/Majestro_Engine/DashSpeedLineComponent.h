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

	float mLifetime = 0.18f;
	float mSampleInterval = 0.01f;
	float mMinSegmentDistance = 10.0f;
	float mHeightOffset = 75.0f;
	float mBackOffset = 35.0f;

	uint32 mLineCount = 5;
	float mLineWidth = 8.0f;
	float mLineSpread = 50.0f;
	float mBaseAlpha = 0.6f;
	float mIntensity = 2.0f;
	float mUvTiling = 1.0f;
	float mBreakStrength = 0.45f;
	float mLineStrength = 0.75f;

	Vec3 mCoreColor = Vec3(0.75f, 0.95f, 1.0f);
	Vec3 mEdgeColor = Vec3(0.15f, 0.45f, 1.0f);
	Vec3 mSubColor = Vec3(0.05f, 0.12f, 0.55f);

	std::wstring mTextureName;

	std::vector<DashSpeedLineSample> mSamples;
	float mTimeSinceLastSample = 0.0f;

	
	bool mHasLastSourcePosition = false;
	Vec3 mLastSourcePosition{};
};
