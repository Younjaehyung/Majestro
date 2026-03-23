#pragma once
#include "System.h"
#include "World.h"
#include "UITransformComponent.h"

// UI 버튼 입력 처리 시스템 (Post 페이즈)
// UITransformSystem이 mFinalPixelPos를 계산한 후 실행됨
// 담당:
//   - 마우스 AABB 히트테스트 (Pivot 보정 포함)
//   - 호버/클릭 콜백 발화
//   - UICusSpriteComponent 머티리얼 Diffuse 색상 갱신
//   - UITransformComponent 크기 스케일 갱신
//   - UITextComponent mFontPos를 버튼 중심 좌표로 갱신
class UIButtonSystem : public System
{
public:
    UIButtonSystem(World* world);
    void Initialize() override;
    void Update(float dt) override;

    std::vector<std::type_index> After() const override;

private:
    // Pivot 보정 AABB 히트테스트
    // mFinalPixelPos는 pivot 기준 좌표이므로 topLeft = pos - pivot * size
    bool HitTest(const Vec2& finalPixelPos, const Vec2& pivot,
                 const Vec2& size, const Vec2& mousePos) const;
};
