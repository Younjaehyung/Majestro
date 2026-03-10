#pragma once
#include "Entity.h"
#include "Component.h"

enum class InputButtons : uint8 {
    NONE,
    SPACE,
    SHIFT,
    Q,
    E,
    ATTACK,
    SKILL1,
    SKILL2,
    RELOAD,
    SPECIAL,
    END
};

enum class InputMouse : uint8 {
    NONE,
    LEFT,
	RIGHT,
	WHEEL,
    END
};

class InputComponent : public Component<InputComponent>
{
public:
    // 이번 서버 틱에서 사용할 “현재 입력 상태”
    float MoveX = 0.0f;
    float MoveY = 0.0f;
	float MoveZ = 0.0f;
    
    uint32 Buttons = 0;
	uint8  Mouse = 0;



    float Yaw = 0.0f;
    float Pitch = 0.0f;

    uint32 lastSeq = 0;

public:
    // InputButtons 버튼 마스킹 
    // 마스킹에 따라 현재 누른 버튼을 알 수 있음
    void PressButton(InputButtons button) {
        Buttons |= (1 << static_cast<uint8>(button));
	}

    void ReleaseButton(InputButtons button) {
        Buttons &= ~(1 << static_cast<uint8>(button));
	}

    bool IsButtonPressed(InputButtons button) const {
        return (Buttons & (1 << static_cast<uint8>(button))) != 0;
	}
public:
    void PressMouse(InputMouse button) {
        Mouse |= (1 << static_cast<uint8>(button));
    }
    void ReleaseMouse(InputMouse button) {
        Mouse &= ~(1 << static_cast<uint8>(button));
    }
    bool IsMousePressed(InputMouse button) const {
        return (Mouse & (1 << static_cast<uint8>(button))) != 0;
	}

};

