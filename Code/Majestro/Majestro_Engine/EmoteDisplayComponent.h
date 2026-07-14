#pragma once

#include "Component.h"
#include "Entity.h"

class EmoteDisplayComponent : public Component<EmoteDisplayComponent>
{
public:
	Entity mCaster = NULL_ENTITY;
	uint8 mEmoteId = 0;
	float mElapsed = 0.0f;
	float mLifetime = 2.5f;
};
