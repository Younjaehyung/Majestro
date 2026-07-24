#pragma once
#include "UIFeature.h"

// 관문지기
class UILevelSelectFeature : public UIFeature
{
public:
    void Initialize(World* world) override;
    void Update(float dt) override;

public:
    static constexpr int32 kStageCount = 4;   // STAGE 01~04

private:
    void SetVisible(bool visible);
    void ApplySelection(int32 stage);

    Entity mBackground{};        // 풀스크린 오버레이 (LEVEL 타이틀)
    Entity mStageCard{};         // 선택된 스테이지 정보 카드 (좌측)
    Entity mStageButtons[kStageCount]{};   // STAGE 01~04 버튼 (우측)
    Entity mGoButton{};          // GO! 레벨 진입 확정 버튼

    bool  mVisible = false;
    int32 mAppliedStage = -1;    // 카드/버튼 틴트를 갱신한 마지막 선택
};
