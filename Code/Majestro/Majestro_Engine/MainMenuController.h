#pragma once
#include "Component.h"
#include "Entity.h"


enum class MainMenuState : int
{
    Title    = 0,
    MainMenu = 1,
    Setting  = 2,
    Manual   = 3,
    RoomList = 4,
    Exit     = 5,
    Count
};

// 메인메뉴 FSM 보유 컴포넌트. 카메라 entity 에 부착.
// 상태 전환은 Request() 로 예약
class MainMenuController : public Component<MainMenuController>
{
public:
    MainMenuState mState        = MainMenuState::Title;
    MainMenuState mPendingState = MainMenuState::Title;
    bool          mHasPending   = false;

    // 상태별 가시화할 UI entity 목록 (MainMenuSystem 이 visible/enabled 토글)
    std::array<std::vector<Entity>, (size_t)MainMenuState::Count> mStateEntities;

    // 상태별 배경 스프라이트
    std::array<Entity, (size_t)MainMenuState::Count> mStateBackground;

    // 배경 페이드+줌 진행 상태
    Entity mActiveBg{};            // 현재 페이드 대상 (무효 = 없음)
    float  mBgFadeT        = 0.f;
    float  mBgFadeDuration = 0.05f;
    float  mBgTargetAlpha  = 1.0f; // 텍스처 자체 알파에 곱할 최대치

    void Request(MainMenuState next)
    {
        if (next == mState && !mHasPending) return;
        mPendingState = next;
        mHasPending   = true;
    }
};
