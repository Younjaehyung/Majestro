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

    // 시점별 상태
    struct StateVisuals
    {
        std::vector<Entity> entities;     // 버튼/텍스트
        Entity              background{};  // 배경 스프라이트 1개
        std::vector<Entity> animations;   // 장식 애니메이션
    };
    std::array<StateVisuals, (size_t)MainMenuState::Count> mStates;

    // 배경 페이드+줌 진행 상태
    Entity mActiveBg{};            // 현재 페이드 대상 (무효 = 없음)
    float  mBgFadeT        = 0.f;
    float  mBgFadeDuration = 0.15f;
    float  mBgTargetAlpha  = 1.0f; // 텍스처 자체 알파에 곱할 최대치

    // true 면 카메라 전환 완료 후 현재 상태 UI 를 한 번 노출
    bool   mEntitiesPendingReveal = false;

    void Request(MainMenuState next)
    {
        if (next == mState && !mHasPending) return;
        mPendingState = next;
        mHasPending   = true;
    }
};
