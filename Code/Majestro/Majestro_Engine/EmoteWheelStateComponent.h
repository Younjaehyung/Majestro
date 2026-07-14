#pragma once

#include "Component.h"

class EmoteWheelStateComponent : public Component<EmoteWheelStateComponent>
{
public:
	bool mIsOpen = false;
	int mSelectedEmoteId = -1;
	Vec2 mSelectionOffset = Vec2::Zero;
	float mOpenElapsed = 0.0f;
};
