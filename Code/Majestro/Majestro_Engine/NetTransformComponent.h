#pragma once
#include "Component.h"

class NetTransformComponent : public Component<NetTransformComponent>
{
public:
	Vec3 mStartPosition{};
	Vec3 mTargetPosition{};
	Vec3 mStartRotation{};
	Vec3 mTargetRotation{};

	float mElapsed = 0.0f;
	float mDuration = 0.1f;
	bool mHasTarget = false;
	bool mInitialized = false;
	uint32 mLastSequence = 0;
};