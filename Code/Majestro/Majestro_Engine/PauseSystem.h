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

    bool mPrevPaused     = false;  // 마우스룩 토글 엣지 감지용
    bool mSavedMouseLook = false;  // 일시정지 진입 전 마우스룩 상태 (복귀 시 복원)
};
