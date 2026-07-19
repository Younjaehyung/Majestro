#pragma once
#include "Component.h"

class ChatComponent : public Component<ChatComponent>
{
public:
	float mNextChatTime = 0.f;	// 도배 방지: 이 서버 시각 전엔 채팅 불가
};
