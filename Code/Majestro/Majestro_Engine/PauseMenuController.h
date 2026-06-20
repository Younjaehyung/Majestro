#pragma once
#include "Component.h"
#include "Entity.h"


enum class PauseMenuState : int
{
    Hidden            = 0,  // 게임 진행 중, 메뉴 없음
    Root              = 1,  // Resume / Setting / Manual / Back To Title
    Manual            = 2,
    SettingGraphics   = 3,  // Setting > Graphics 탭
    SettingSound      = 4,  // Setting > Sound 탭
    ConfirmDisconnect = 5,  // "나가시겠습니까?" Yes / No
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

    // Pause 루트 버튼과 각 버튼 위에 표시하는 호버 선 엔티티다.
    std::array<Entity, 4> mRootButtons{};
    std::array<Entity, 4> mRootHoverLines{};

    // 상단 두 장과 하단 두 장을 이어 붙여 Pause 문구 띠를 반복 이동시킨다.
    std::array<Entity, 4> mWordBandEntities{};
    float mWordBandOffset = 0.0f;
    float mWordBandTileWidth = 2560.0f;
    float mWordBandScrollSpeed = 120.0f;
    float mWordBandTopY = -48.0f;
    float mWordBandBottomY = 1312.0f;

    void Request(PauseMenuState next)
    {
        mPendingState = next;
        mHasPending   = true;
    }
};
