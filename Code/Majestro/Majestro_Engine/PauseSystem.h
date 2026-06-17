#pragma once
#include "System.h"
#include "World.h"

// 인게임 일시정지 토글 + 메뉴 상태 전환 시스템 
class PauseSystem : public System
{
public:
    PauseSystem(World* world);

    void Update(float dt) override;

    std::vector<std::type_index> Before() const override;

private:
    void SetEntitiesVisible(const std::vector<class Entity>& es, bool visible);

    // 게임 렌더 파이프라인의 풀스크린 일시정지 blur 토글
    void SetPauseBlur(bool on);

    // 로컬 캐릭터(mPlayerType)에 맞춰 배경 레이어 텍스처 교체
    void ApplyBackgroundTexture(class PauseMenuController* ctrl);

    bool mPrevPaused     = false;  // 마우스룩 토글 엣지 감지용
    bool mSavedMouseLook = false;  // 일시정지 진입 전 마우스룩 상태 (복귀 시 복원)
};
