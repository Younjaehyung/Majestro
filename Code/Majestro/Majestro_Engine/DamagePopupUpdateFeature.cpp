#include "pch.h"
#include "DamagePopupUpdateFeature.h"

#include "DamagePopupComponent.h"
#include "UITextComponent.h"
#include "UITransformComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "EnemyComponent.h"
#include "GameEvents.h"
#include "World.h"
#include "Engine.h"
#include "RenderManager.h"
#include "MathUtils.h"

namespace
{
    constexpr float SPAWN_JITTER_X = 40.f; // 같은 프레임 다중 피격 시 가시성을 위한 X 오프셋 흔들기
    constexpr float SMOOTHSTEP_LO = 0.6f;
    constexpr float SMOOTHSTEP_HI = 1.0f;
    constexpr float POP_IN_LO = 0.0f;
    constexpr float POP_IN_HI = 0.15f;
    constexpr float POP_IN_SCALE = 0.3f;
    constexpr float FADE_OUT_SCALE = 0.2f;

    constexpr float BASE_SCALE = 1.0f;
    constexpr float CRITICAL_SCALE_MUL = 1.3f; // 크리티컬 히트 글자 크기 배수
    constexpr float NORMAL_OUTLINE_PX = 2.0f;   // 일반 피격 외곽선 두께(px)
    constexpr float CRITICAL_OUTLINE_PX = 3.0f; // 크리티컬 외곽선 두께(px)

}

void DamagePopupUpdateFeature::Update(float dt)
{
    if (mWorld == nullptr) return;
    ConsumeEvents();
    UpdatePopups(dt);
}

void DamagePopupUpdateFeature::ConsumeEvents()
{
    mWorld->GetEventManager()->Consume<EvHealthChanged>([&](const EvHealthChanged& e)
    {
        const int32 dmg = e.previousHp - e.hp;
        if (dmg <= 0) return; // 힐/변동 없음 무시

        // 적 피격만 팝업 — 플레이어 본인 HP 변화는 별도 HUD가 담당
        if (!mWorld->GetComponent<EnemyComponent>(e.target)) return;

        TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(e.target);
        if (!targetTr) return;

        Entity popup = mWorld->CreateEntity();

        DamagePopupComponent& dmgComp = mWorld->AddComponent<DamagePopupComponent>(popup);
        dmgComp.mAnchor = e.target;
        dmgComp.mDamageValue = dmg;
        dmgComp.mIsCritical = e.isCritical;

        // X 방향으로 살짝 무작위 흔들기 — 연속 피격이 겹쳐 보이지 않게
        const float jitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * SPAWN_JITTER_X;
        dmgComp.mWorldOffset.x += jitter;

        dmgComp.mLastWorldPos = targetTr->mLocalPosition + dmgComp.mWorldOffset;
        dmgComp.mHasLastWorldPos = true;

        // UITransformComponent — 위치/피벗/스케일을 담당. 매 프레임 mFinalPixelPos/mScale을 직접 갱신.
        UITransformComponent& tr = mWorld->AddComponent<UITransformComponent>(popup);
        tr.mLayoutMode = UILayoutMode::Pixel;
        tr.mAnchor = Anchor::TopLeft;
        tr.mPivot = Vec2(0.5f, 0.5f);
        tr.mScale = Vec2(1.f, 1.f);

        UITextComponent& text = mWorld->AddComponent<UITextComponent>(popup);
        text.mText = std::to_wstring(dmg);
        if (e.isCritical) text.mText += L"!"; // 크리티컬 강조

        // 외곽선
        text.mOutlineThickness = e.isCritical ? CRITICAL_OUTLINE_PX : NORMAL_OUTLINE_PX;
        text.mOutlineColor = e.isCritical
            ? DirectX::XMVECTORF32{ { { 1.f, 1.f, 1.f, 1.f } } }   // 흰색
            : DirectX::XMVECTORF32{ { { 0.f, 0.f, 0.f, 1.f } } };  // 검정
        text.mVisible = false; // 다음 Update에서 투영 후 켜진다
    });
}

void DamagePopupUpdateFeature::UpdatePopups(float dt)
{
    if (!mWorld->HasComponentPool<DamagePopupComponent>()) return;

    // 카메라 1회 조회
    if (!mWorld->HasComponentPool<MainCameraComponent>()) return;
    auto cameras = mWorld->GetEntitiesWithComponent<MainCameraComponent>();
    if (cameras.empty()) return;
    CameraComponent* camera = mWorld->GetComponent<CameraComponent>(cameras[0]);
    if (!camera) return;

    const WindowInfo& window = RENDERMANAGER.GetWindow();
    const float vpW = static_cast<float>(window.Width);
    const float vpH = static_cast<float>(window.Height);

    std::vector<Entity> expired;
    for (Entity e : mWorld->View<DamagePopupComponent>())
    {
        DamagePopupComponent* dmg = mWorld->GetComponent<DamagePopupComponent>(e);
        UITextComponent* text = mWorld->GetComponent<UITextComponent>(e);
        UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(e);
        if (!dmg || !text || !tr)
        {
            expired.push_back(e);
            continue;
        }

        dmg->mAge += dt;
        if (dmg->mAge >= dmg->mLifetime)
        {
            expired.push_back(e);
            continue;
        }

        // 월드 위치 결정
        Vec3 baseWorld = dmg->mLastWorldPos;
        if (TransformComponent* anchorTr = mWorld->GetComponent<TransformComponent>(dmg->mAnchor))
        {
            baseWorld = anchorTr->mLocalPosition + dmg->mWorldOffset;
            dmg->mLastWorldPos = baseWorld;
            dmg->mHasLastWorldPos = true;
        }
        const Vec3 worldPos = baseWorld + dmg->mFloatVelocity * dmg->mAge;

        // 스크린 투영
        Vec2 screenPos{};
        const bool visible = ProjectWorldToScreen(worldPos,camera->GetViewMatrix(),
                               camera->GetProjectionMatrix(), vpW, vpH, screenPos);
        text->mVisible = visible;
        if (!visible) continue;

        // UITransformSystem이 이미 이번 프레임 mFinalPixelPos을 계산했지만, 월드 투영 결과로 덮어씀
        tr->mFinalPixelPos = screenPos;

        // 페이드 / 스케일 애니메이션
        const float t = dmg->mAge / dmg->mLifetime;
        const float alpha = 1.f - MathUtils::SmoothStep(SMOOTHSTEP_LO, SMOOTHSTEP_HI, t);
        float scale = BASE_SCALE
                            + POP_IN_SCALE * MathUtils::SmoothStep(POP_IN_LO, POP_IN_HI, t)
                            - FADE_OUT_SCALE * MathUtils::SmoothStep(SMOOTHSTEP_LO, SMOOTHSTEP_HI, t);

        // 크리티컬은 글자 크기 강조
        if (dmg->mIsCritical)
            scale *= CRITICAL_SCALE_MUL;

        
        if (dmg->mIsCritical)
            text->mColor = DirectX::XMVECTORF32{ { { 1.f, 0.25f, 0.15f, alpha } } };
        else
            text->mColor = DirectX::XMVECTORF32{ { { 1.f, 0.85f, 0.2f, alpha } } };
        tr->mScale = Vec2(scale, scale);
    }

    for (Entity e : expired)
        mWorld->DestroyEntity(e);
}

// worldPos를 화면 픽셀 좌표로 투영. 반환값 false면 카메라 뒤쪽이라 표시하지 말 것.
bool DamagePopupUpdateFeature::ProjectWorldToScreen(const Vec3& worldPos,
    const Matrix& view,
    const Matrix& proj,
    float viewportW,
    float viewportH,
    Vec2& outScreenPos)
{

    const DirectX::XMVECTOR wp = DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.f);
    const DirectX::XMVECTOR vp = DirectX::XMVector4Transform(wp, view);
    const DirectX::XMVECTOR cp = DirectX::XMVector4Transform(vp, proj);

    DirectX::XMFLOAT4 clip;
    DirectX::XMStoreFloat4(&clip, cp);

    if (clip.w <= 0.0001f) return false;

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;

    outScreenPos.x = (ndcX * 0.5f + 0.5f) * viewportW;
    outScreenPos.y = (1.f - (ndcY * 0.5f + 0.5f)) * viewportH;

    if (ndcX < -1.2f || ndcX > 1.2f || ndcY < -1.2f || ndcY > 1.2f)
        return false;
    return true;
}
