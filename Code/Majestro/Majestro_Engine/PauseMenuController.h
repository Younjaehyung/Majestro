#pragma once
#include "Component.h"
#include "Entity.h"


enum class PauseMenuState : int
{
    Hidden            = 0,  // 게임 진행 중, 메뉴 없음
    Root              = 1,  // Resume / Setting / Disconnect
    SettingGraphics   = 2,  // Setting > Graphics 탭
    SettingSound      = 3,  // Setting > Sound 탭
    ConfirmDisconnect = 4,  // "나가시겠습니까?" Yes / No
    Count
};

// 인게임 일시정지 메뉴 FSM 컴포넌트. 전용 entity 1개에 부착(싱글톤 성격).
class PauseMenuController : public Component<PauseMenuController>
{
public:
    bool           mPaused       = false;   // PlayerInputSystem 이 로컬 입력을 차단할지 판단하는 플래그.
    PauseMenuState mState        = PauseMenuState::Hidden;
    PauseMenuState mPendingState = PauseMenuState::Hidden;
    bool           mHasPending   = false;

    // 상태별 가시화할 UI entity 목록 (PauseSystem 이 visible/enabled 토글)
    std::array<std::vector<Entity>, (size_t)PauseMenuState::Count> mStateEntities;

    // 캐릭터별 전체화면 배경 레이어 sprite
    Entity mBackgroundEntity = NULL_ENTITY;

    void Request(PauseMenuState next)
    {
        mPendingState = next;
        mHasPending   = true;
    }
};
