#pragma once

#include "Component.h"

class EmoteComponent : public Component<EmoteComponent>
{
public:
	// 서버가 마지막으로 승인한 감정표현 요청 시각
	float mLastRequestTime = -1000.0f;
};
