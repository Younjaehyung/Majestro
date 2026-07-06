#pragma once
#include "UIFeature.h"

// 광장 NPC 대화 UI.
class UIDialogueFeature : public UIFeature
{
public:
    void Initialize(World* world) override;
    void Update(float dt) override;

private:
    Entity mPromptText{};   // 근접 프롬프트
    Entity mTalkBg{};       // 톡박스 배경 (풀스크린 오버레이)
    Entity mPortrait{};     // 캐릭터 초상화 (좌측 하단)
    Entity mNameText{};     // NPC 이름
    Entity mLineText{};     // 현재 대사
    Entity mHintText{};     // "[F] 다음"
};
