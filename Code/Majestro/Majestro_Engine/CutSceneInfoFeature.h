#pragma once
#include "UIFeature.h"

// 씬 진입 시네마틱 카메라가 재생되는 동안에만 화면 좌하단에 맵 소개 카드를 그린다.
// 아틀라스 Ui_CutScene_Info_Sheet(2048x2048)는 1024x512 셀 8칸으로 나뉘며,
// 검정 잉크 wash / 흰 선 / 맵별 텍스트 세 장이 같은 캔버스에 정합되어 있어
// 하나의 목적지 사각형에 겹쳐 그리면 원본 디자인 그대로 합성된다.
class CutSceneInfoFeature : public UIFeature
{
public:
    void SpriteRender(DirectX::SpriteBatch* spriteBatch) override;
    bool RendersInGroup(UIRenderGroup group) const override { return group == UIRenderGroup::Cinematic; }

private:
    // 화면 대비 카드 크기 (카드 원본 비율 1024:512 = 2:1 유지).
    // 비율 기준이라 해상도가 달라져도 같은 위치·같은 상대 크기로 나온다.
    static constexpr float kCardHeightRatio = 0.30f;   // 화면 높이 대비 카드 높이

    // 1024x512 셀 안에서 그림이 실제로 차지하는 범위(실측).
    // 좌측은 x=0 부터 채워져 있지만 하단은 y=478 아래가 투명 여백이라,
    // 그 여백만큼 카드를 내려야 그림이 화면 좌하단 모서리에 딱 붙는다.
    static constexpr float kContentLeftRatio   = 0.f;          // 0 / 1024
    static constexpr float kContentBottomRatio = 479.f / 512.f;

    // 등장/퇴장 연출
    static constexpr float kFadeInDur  = 0.35f;  // 시네마틱 시작 후 페이드 인
    static constexpr float kFadeOutDur = 0.60f;  // 시네마틱 종료 전 페이드 아웃
    static constexpr float kSlideInDur = 0.55f;  // 좌측에서 밀려 들어오는 시간
};
