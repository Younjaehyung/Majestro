#pragma once
#include <string>
#include "Component.h"
#include "Entity.h"

class SocketFollowComponent : public Component<SocketFollowComponent>
{
public:
	Entity mSourceEntity{};
	std::string mSocketName;


	Vec3 mSocketLocalOffset = Vec3::Zero;
	Vec3 mWorldOffset = Vec3::Zero;

	
	bool mFollowRotation = true;
	bool mHideWhenInvalid = true;
	Vec3 mHiddenPosition = Vec3(0.0f, -100000.0f, 0.0f);


	bool mUseAttackState = false;
	bool mUseAnimationWindow = false;
	bool mUseUpperAnimationWindow = true;
	bool mUseAnimationFrameWindow = false;
	float mTrailStartTime = 0.0f;
	float mTrailEndTime = 0.0f;
	uint32 mTrailStartFrame = 0;
	uint32 mTrailEndFrame = 0;


	bool mRestartVfxOnActivate = true;
	bool mWasActive = false;
	bool mPlayOnce = false;
};
