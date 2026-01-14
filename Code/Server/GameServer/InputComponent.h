#pragma once
#include "Entity.h"
#include "Component.h"

class InputComponent : public Component<InputComponent>
{
public:
    // 이번 서버 틱에서 사용할 “현재 입력 상태”
    float moveX = 0.0f;
    float moveY = 0.0f;
    uint8 buttons = 0;
    float yaw = 0.0f;
    float pitch = 0.0f;

    uint32_t lastSeq = 0; // 중복/역순 입력 방지
};

