#include "pch.h"
#include "UIHpBarUpdateFeature.h"

#include "Engine.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "RenderComponent.h"
#include "TransformComponent.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "UIComponent.h"
#include "UIRenderSystem.h" // UIInstanceData
#include "UITransformComponent.h"
#include "MathUtils.h"

void UIHpBarUpdateFeature::Update(float dt)
{
    UpdateHpBarUI(dt);
}


void UIHpBarUpdateFeature::WorldRender(CameraComponent* camera)
{
	DrawUI(camera, WorldUIPassMode::World);
}

void UIHpBarUpdateFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{
    if (mWorld->HasComponentPool<MainCameraComponent>() == false) return;

    auto cameras = mWorld->GetEntitiesWithComponent<MainCameraComponent>();
    if (cameras.empty()) return;

    CameraComponent* camera = mWorld->GetComponent<CameraComponent>(cameras[0]);
    if (camera == nullptr) return;

    DrawUI(camera, WorldUIPassMode::HUD);
}



void UIHpBarUpdateFeature::DrawUI(CameraComponent* camera, WorldUIPassMode mode)
{
    if (mWorld == nullptr || camera == nullptr)
        return;

    if (mWorld->HasComponentPool<UIHpBarComponent>() == false)
        return;

    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN))
        .OMSetRenderTargets(1, backIndex);

    RENDERMANAGER.SetGraphicsTable();

    for (Entity owner : mWorld->GetEntitiesWithComponent<UIHpBarComponent>())
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(owner);
        if (hpBar == nullptr)
            continue;
        if (mode == WorldUIPassMode::World && hpBar->mIsScreenSpace) continue;
        if (mode == WorldUIPassMode::HUD && !hpBar->mIsScreenSpace) continue;

        if (hpBar->mTargetEntity == NULL_ENTITY)
            hpBar->mTargetEntity = owner;

        DrawHpBar(hpBar, owner);
    }


    const uint32 zero = 0;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);  // BaseInstanceID
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);  // 스프라이트 역할
}

void UIHpBarUpdateFeature::DrawHpBar(UIHpBarComponent* hpBar, Entity owner)
{
    // 모드별 앵커 결정
    // 월드 모드와 HUD 모드가 같은 앵커 슬롯을 사용
    //   World: 월드 좌표 (xyz 모두 사용)
    //   HUD:   화면 픽셀 좌상단 + (mMaxWidth/2, 0) — 좌상단을 가운데-위로 보정해
    //          기존 pivot=(-w/2, 0) 수식을 모드 무관하게 그대로 통용 (z 미사용).
    Vec3 anchorXYZ = Vec3::Zero;
    if (hpBar->mIsScreenSpace)
    {
        UITransformComponent* uiTr = mWorld->GetComponent<UITransformComponent>(owner);
        if (uiTr == nullptr)
            return; // HUD 모드인데 UITransform 없음 → 무시
        const Vec2 topLeft = uiTr->mFinalPixelPos;
        anchorXYZ = Vec3(topLeft.x + hpBar->mMaxWidth * 0.5f, topLeft.y, 0.f);
    }
    else
    {
        TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(hpBar->mTargetEntity);
        if (targetTr == nullptr)
            return;
        RenderComponent* targetRender = mWorld->GetComponent<RenderComponent>(hpBar->mTargetEntity);
        if (targetRender != nullptr && targetRender->mVisibility == false)
            return;
        const Vec3 worldAnchor = targetTr->mLocalPosition + hpBar->mWorldOffset;
        anchorXYZ = worldAnchor;
    }

    // 텍스처 인덱스 확보
    shared_ptr<Texture> bgTex = RESOURCEMANAGER.Get<Texture>(hpBar->mBackgroundMaterialName);
    shared_ptr<Texture> fillTex = RESOURCEMANAGER.Get<Texture>(hpBar->mFillMaterialName);
    // bg/fill 을 그리지 않을 거라면 텍스처 누락이어도 통과 (HUD 가 자체 sprite 로 그릴 때).
    if (hpBar->mRenderBgFill)
    {
        if (bgTex == nullptr)
            return;
        if (fillTex == nullptr)
            fillTex = bgTex;
    }

    shared_ptr<Texture> hitTex = nullptr;
    if (hpBar->mHitEffectTextureName.empty() == false)
        hitTex = RESOURCEMANAGER.Get<Texture>(hpBar->mHitEffectTextureName);

    // 쉴드(아머) 바 데이터 — mShieldMaterialName 지정 + 쉴드>0 일 때만 그린다.
    shared_ptr<Texture> shieldTex = nullptr;
    float shieldEndRatio = 0.f;   // (curHp+shield)/denom — 쉴드 우측 끝 (하우징 분율, 0~1)
    if (hpBar->mRenderBgFill && hpBar->mShieldMaterialName.empty() == false)
    {
        HealthComponent* h = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);
        ArmorComponent*  a = mWorld->GetComponent<ArmorComponent>(hpBar->mTargetEntity);
        if (h != nullptr && h->mMaxHp > 0 && a != nullptr)
        {
            const int32 maxHp = h->mMaxHp;
            const int32 curHp = std::clamp(h->mCurrentHp, 0, maxHp);
            const int32 shieldAmt = (std::max)(0, a->mCurrentArmor);
            if (shieldAmt > 0)
            {
                const float denom = static_cast<float>((std::max)(maxHp, curHp + shieldAmt));
                shieldEndRatio = static_cast<float>(curHp + shieldAmt) / denom;
                shieldTex = RESOURCEMANAGER.Get<Texture>(hpBar->mShieldMaterialName);
            }
        }
    }

    // HP 바 셰이더 파라미터
    UIHpBarParamsLayout gp{};
    gp.BaseInstanceID = 0;
    gp.PassFlags = hpBar->mIsScreenSpace ? 1u : 0u;
    gp.SpriteRole = 0;
    gp.ReservedHeader = 0;

    gp.AnchorWorldX = anchorXYZ.x;
    gp.AnchorWorldY = anchorXYZ.y;
    gp.AnchorWorldZ = anchorXYZ.z;

    gp.SizePxX = hpBar->mMaxWidth;
    gp.SizePxY = hpBar->mHeight;

    gp.PivotPxX = -hpBar->mMaxWidth * 0.5f + hpBar->mScreenOffsetPx.x;
    gp.PivotPxY = hpBar->mScreenOffsetPx.y;

    // 텍스처 여백 보정
    gp.FollowRatio = RemapBarRatioToUv(std::clamp(hpBar->mPreviousHpRatio, 0.f, 1.f), hpBar->mFillUvRangeX);
    gp.BackgroundTextureIndex = (bgTex != nullptr) ? bgTex->GetImageIndex() : 0u;
    gp.FillTextureIndex = (fillTex != nullptr) ? fillTex->GetImageIndex() : 0u;
    gp.HitTextureIndex = (hitTex != nullptr) ? hitTex->GetImageIndex() : 0u;

    // packed: cols(0..7) | rows(8..15) | frameCount(16..31)
    const uint32 cols = static_cast<uint32>((std::max)(1, hpBar->mHitEffectCols)) & 0xFFu;
    const uint32 rows = static_cast<uint32>((std::max)(1, hpBar->mHitEffectRows)) & 0xFFu;
    const uint32 frameCount = static_cast<uint32>((std::max)(1, hpBar->mHitEffectFrameCount)) & 0xFFFFu;
    gp.HitConfig = cols | (rows << 8u) | (frameCount << 16u);

    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);

    auto spriteShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIHpSprite");
    auto fragShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIHpFragment");
    auto hitShader = RESOURCEMANAGER.Get<Shader>(L"WorldUIHpHit");
    auto quadMesh = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
    auto triMesh = RESOURCEMANAGER.Get<Mesh>(L"UITriangle");
    if (fragShader == nullptr || quadMesh == nullptr || triMesh == nullptr)
        return;

    //  bg/fill sprite — HUD 의 mRenderBgFill=false 면 스킵
    if (hpBar->mRenderBgFill && spriteShader != nullptr && bgTex != nullptr)
    {
        spriteShader->Update();

        // 쉴드 바 (HP 채움 아래) — 쉴드 텍스처의 색 영역(UV)을 HP 바 하우징에 리맵해
        // 같은 줄에 정렬하고, [0, shieldEndRatio] 까지 그린다. 이후 HP 채움이 [0, curHp/denom]을
        // 위에서 덮어, 쉴드는 "현재 체력 오른쪽 ~ (curHp+쉴드)/denom" 구간만 보인다.
        if (shieldTex != nullptr)
        {
            const Vec2 hux = hpBar->mFillUvRangeX;     // HP 색 영역 X
            const Vec2 huy = hpBar->mFillUvRangeY;     // HP 색 영역 Y
            const Vec2 sux = hpBar->mShieldUvRangeX;   // 쉴드 색 영역 X
            const Vec2 suy = hpBar->mShieldUvRangeY;   // 쉴드 색 영역 Y

            UIHpBarParamsLayout gpS = gp;
            gpS.FillTextureIndex = shieldTex->GetImageIndex();
            // 쉴드 색 영역(sux/suy)이 HP 색 영역(hux/huy) 스크린 위치에 정확히 오도록 스케일/피벗 보정.
            const float sx = (hux.y - hux.x) / (std::max)(1e-4f, sux.y - sux.x) * hpBar->mMaxWidth;
            const float sy = (huy.y - huy.x) / (std::max)(1e-4f, suy.y - suy.x) * hpBar->mHeight;
            gpS.SizePxX = sx;
            gpS.SizePxY = sy;
            gpS.PivotPxX = gp.PivotPxX + hux.x * hpBar->mMaxWidth - sux.x * sx;
            gpS.PivotPxY = gp.PivotPxY + huy.x * hpBar->mHeight - suy.x * sy;
            // 우측 컷오프(셰이더 discard)는 쉴드 UV 공간에서: 하우징 분율 shieldEndRatio → 쉴드 UV
            gpS.FollowRatio = RemapBarRatioToUv(std::clamp(shieldEndRatio, 0.f, 1.f), sux);

            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gpS, 0);
            const uint32 shieldRole = 1;
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &shieldRole, 2);
            quadMesh->Render(1, 0, 0, 0);

            // HP 채움과 배경에 사용할 원본 파라미터를 복원한다
            GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, &gp, 0);
        }

        const uint32 fillRole = 1;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &fillRole, 2);
        quadMesh->Render(1, 0, 0, 0);   // 채움 먼저 (쉴드 위, 배경 아래)

        const uint32 bgRole = 0;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &bgRole, 2);
        quadMesh->Render(1, 0, 0, 0);   // 배경 나중 (위)
    }

    // 파편 + hit effect 인스턴스를 한 번에 업로드
    const UIInstanceRanges ranges = UploadBarInstances(hpBar);

    //파편 삼각형
    if (ranges.FragmentCount > 0)
    {
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &ranges.FragmentStart, 0);
        fragShader->Update();
        triMesh->Render(ranges.FragmentCount, 0, 0, 0);
    }

    // Hit effect 스프라이트 시트 (가산 블렌드)
    if (ranges.HitCount > 0 && hitShader != nullptr && hitTex != nullptr)
    {
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &ranges.HitStart, 0);
        hitShader->Update();
        quadMesh->Render(ranges.HitCount, 0, 0, 0);
    }
}

UIHpBarUpdateFeature::UIInstanceRanges UIHpBarUpdateFeature::UploadBarInstances(const UIHpBarComponent* hpBar)
{
    UIInstanceRanges ranges{};
    if (hpBar == nullptr || hpBar->mLossFragments.empty())
        return ranges;

    std::vector<UIInstanceData> instances;
    instances.reserve(128);

    // 파편 삼각형 인스턴스 (UIQuad 아님, UITriangle 메시)
    ranges.FragmentStart = static_cast<uint32>(instances.size());
    for (const UIHpLossFragment& mFragment : hpBar->mLossFragments)
    {
        if (mFragment.LifeTime <= 0.f)
            continue;
        const float t = std::clamp(mFragment.Age / mFragment.LifeTime, 0.f, 1.f);
        const float alpha = 1.f - t;
        if (alpha <= 0.01f)
            continue;

        for (const UIHpLossTriangle& tri : mFragment.Triangles)
        {
            UIInstanceData d{};
            d.Position = tri.V0;     // V0 픽셀 오프셋
            d.Size = tri.V1;         // V1 픽셀 오프셋
            d.Pivot = tri.V2;        // V2 픽셀 오프셋
            d.MaterialIndex = 0;
            d.ZOrder = alpha;
            instances.push_back(d);
        }
    }
    ranges.FragmentCount = static_cast<uint32>(instances.size()) - ranges.FragmentStart;

    //Hit effect 인스턴스 (UIQuad 메시)
    // 텍스처/그리드 미설정이면 스킵.
    const bool hitEnabled = hpBar->mHitEffectTextureName.empty() == false &&
        hpBar->mHitEffectFrameCount > 0 &&
        hpBar->mHitEffectCols > 0 &&
        hpBar->mHitEffectRows > 0;
    ranges.HitStart = static_cast<uint32>(instances.size());
    if (hitEnabled)
    {
        // hit 스프라이트는 바의 "보이는 높이"에 비례한 정사각 크기로 그린다.
        // (mHeight × fillUvRangeY 폭 = 텍스처 캔버스 안에서 실제 바가 차지하는 화면 높이)
        const float visibleBarH = hpBar->mHeight * (std::max)(0.f, hpBar->mFillUvRangeY.y - hpBar->mFillUvRangeY.x);
        const float hitSidePx = (std::max)(1.f, visibleBarH * hpBar->mHitEffectHeightScale);

        for (const UIHpLossFragment& mFragment : hpBar->mLossFragments)
        {
            if (mFragment.LifeTime <= 0.f)
                continue;
            const float ageRatio = std::clamp(mFragment.Age / mFragment.LifeTime, 0.f, 1.f);
            const float alpha = 1.f - ageRatio;
            if (alpha <= 0.01f)
                continue;

            // 가로(U)만 잃은 피 폭만큼 늘려 "피 깎임" 구간을 덮는다. 높이는 정사각 기준 유지.
            const float hitWidthPx = hitSidePx + mFragment.LostWidthPx * hpBar->mHitEffectWidthLossScale;

            // 셰이더는 중앙 정렬이므로, 중심을 가로 폭 절반만큼 오른쪽으로 밀어
            // 스프라이트 왼쪽 변을 경계점(파편 왼쪽 x0)에 고정하고 잃은 구간(오른쪽)으로만 늘린다.
            UIInstanceData d{};
            d.Position = mFragment.HitAnchorPx + hpBar->mHitEffectOffsetPx + Vec2(hitWidthPx * 0.5f, 0.f);
            d.Size = Vec2(hitWidthPx, hitSidePx);  // 가로=높이+피해폭, 세로=바 높이 비례
            d.Pivot = Vec2(alpha, ageRatio);       // x=알파, y=시트 프레임 산출용
            d.MaterialIndex = 0;
            d.ZOrder = 0.f;
            instances.push_back(d);
        }
    }
    ranges.HitCount = static_cast<uint32>(instances.size()) - ranges.HitStart;

    if (instances.empty())
        return ranges;

    if (instances.size() > kUIInfoCapacity)
        instances.resize(kUIInfoCapacity);

    const uint32 frameIdx = RENDERMANAGER.GetFrameResourceIndex();
    RENDERMANAGER.GetGroupBuffer(frameIdx)->UIInfo->PushGraphicsData(
        instances.data(),
        static_cast<uint32>(sizeof(UIInstanceData) * instances.size()));

    return ranges;
}

void UIHpBarUpdateFeature::SpawnHpLossFragments(UIHpBarComponent* hpBar, float oldRatio, float newRatio)
{
    if (hpBar == nullptr)
        return;

    const float clampedOld = std::clamp(oldRatio, 0.f, 1.f);
    const float clampedNew = std::clamp(newRatio, 0.f, 1.f);
    if (clampedNew >= clampedOld)
        return;

    // 잃은 영역의 픽셀 단위 크기(앵커 기준)
    // 텍스처 여백 보정
    const float lostLocalStart = hpBar->mMaxWidth * RemapBarRatioToUv(clampedNew, hpBar->mFillUvRangeX);
    const float lostLocalEnd = hpBar->mMaxWidth * RemapBarRatioToUv(clampedOld, hpBar->mFillUvRangeX);
    const float lostWidth = lostLocalEnd - lostLocalStart; 
	if (lostWidth <= 1.f || hpBar->mHeight <= 0.f)  // 너무 작은 피해는 파편 생략
        return;

    // 앵커 기준 오프셋 (바 좌상단 = (-mMaxWidth/2, 0))
    const float x0 = -hpBar->mMaxWidth * 0.5f + hpBar->mScreenOffsetPx.x + lostLocalStart;
    const float x1 = -hpBar->mMaxWidth * 0.5f + hpBar->mScreenOffsetPx.x + lostLocalEnd;
 
    const float y0 = hpBar->mScreenOffsetPx.y + hpBar->mHeight * hpBar->mFillUvRangeY.x;
    const float y1 = hpBar->mScreenOffsetPx.y + hpBar->mHeight * hpBar->mFillUvRangeY.y;

    // 파편 이벤트 용량 관리
    if (static_cast<int>(hpBar->mLossFragments.size()) + 1 > hpBar->mMaxFragmentCount)
    {
        const int removeCount = static_cast<int>(hpBar->mLossFragments.size()) + 1 - hpBar->mMaxFragmentCount;
        hpBar->mLossFragments.erase(
            hpBar->mLossFragments.begin(),
            hpBar->mLossFragments.begin() + std::min<int>(removeCount, static_cast<int>(hpBar->mLossFragments.size())));
    }

    // 분할
    const int cols = std::clamp(static_cast<int>(std::round(lostWidth / 25.f)), 1, 8);
    const int rows = 2;

    const float cellW = lostWidth / static_cast<float>(cols);
    const float cellH = (y1 - y0) / static_cast<float>(rows);
    const float jitterScaleX = 0.3f * cellW;
    const float jitterScaleY = 0.3f * cellH;

    std::vector<Vec2> grid(static_cast<size_t>(cols + 1) * (rows + 1));
    auto gridAt = [&grid, cols](int i, int j) -> Vec2& {
        return grid[static_cast<size_t>(j) * (cols + 1) + i];
    };

    for (int j = 0; j <= rows; ++j)
    {
        for (int i = 0; i <= cols; ++i)
        {
            const float baseX = x0 + static_cast<float>(i) * cellW;
            const float baseY = y0 + static_cast<float>(j) * cellH;
            const bool edgeX = (i == 0 || i == cols);
            const bool edgeY = (j == 0 || j == rows);
            const float jx = edgeX ? 0.f : MathUtils::RandomRange(-jitterScaleX, jitterScaleX);
            const float jy = edgeY ? 0.f : MathUtils::RandomRange(-jitterScaleY, jitterScaleY);
            gridAt(i, j) = Vec2(baseX + jx, baseY + jy);
        }
    }

    mFragment.Triangles.clear();
    mFragment.Triangles.reserve(static_cast<size_t>(cols) * rows * 2);
    mFragment.LifeTime = hpBar->mFragmentLifeTime;
    mFragment.Age = 0.f;
    mFragment.HitAnchorPx = Vec2(x0, (y0 + y1) * 0.5f);
    mFragment.LostWidthPx = lostWidth;
   

    for (int j = 0; j < rows; ++j)
    {
        for (int i = 0; i < cols; ++i)
        {
            const Vec2 tl = gridAt(i, j);
            const Vec2 tr = gridAt(i + 1, j);
            const Vec2 br = gridAt(i + 1, j + 1);
            const Vec2 bl = gridAt(i, j + 1);

            if (MathUtils::RandomRange(0.f, 1.f) < 0.5f)
            {
                mFragment.Triangles.push_back({ tl, tr, br });
                mFragment.Triangles.push_back({ tl, br, bl });
            }
            else
            {
                mFragment.Triangles.push_back({ tl, tr, bl });
                mFragment.Triangles.push_back({ tr, br, bl });
            }
        }
    }

    hpBar->mLossFragments.push_back(std::move(mFragment));
}

void UIHpBarUpdateFeature::UpdateHpLossFragments(UIHpBarComponent* hpBar, float dt)
{
    if (hpBar == nullptr || hpBar->mLossFragments.empty())
        return;

    for (UIHpLossFragment& mFragment : hpBar->mLossFragments)
        mFragment.Age += dt;

    hpBar->mLossFragments.erase(
        std::remove_if(hpBar->mLossFragments.begin(), hpBar->mLossFragments.end(),
            [](const UIHpLossFragment& mFragment)
            {
                return mFragment.Age >= mFragment.LifeTime;
            }),
        hpBar->mLossFragments.end());
}

void UIHpBarUpdateFeature::UpdateHpBarUI(float dt)
{
    if (mWorld->HasComponentPool<UIHpBarComponent>() == false ||
        mWorld->HasComponentPool<HealthComponent>() == false)
    {
        return;
    }

    for (Entity owner : mWorld->GetEntitiesWithComponent<UIHpBarComponent>())
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(owner);
        if (hpBar == nullptr)
            continue;

        if (hpBar->mTargetEntity == NULL_ENTITY)
            hpBar->mTargetEntity = owner;

        if (hpBar->mIsScreenSpace)
        {
            if (UITransformComponent* uiTransform = mWorld->GetComponent<UITransformComponent>(owner))
            {
                hpBar->mMaxWidth = uiTransform->mFinalSize.x;
                hpBar->mHeight = uiTransform->mFinalSize.y;
            }
        }

        HealthComponent* followHealth = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);
        if (followHealth == nullptr)
            continue;

        // 쉴드 바가 있는 바(적)는 오버실드 방식 denom = max(MaxHp, curHp+쉴드).
        // 쉴드 바가 없으면 shield=0 → denom=MaxHp 로 기존 동작 유지.
        const int32 maxHp = (std::max)(1, followHealth->mMaxHp);
        const int32 curHp = std::clamp(followHealth->mCurrentHp, 0, maxHp);
        int32 shield = 0;
        if (hpBar->mShieldMaterialName.empty() == false)
        {
            ArmorComponent* followArmor = mWorld->GetComponent<ArmorComponent>(hpBar->mTargetEntity);
            if (followArmor != nullptr)
                shield = (std::max)(0, followArmor->mCurrentArmor);
        }
        const float denom = static_cast<float>((std::max)(maxHp, curHp + shield));
        const float followRatio = std::clamp(static_cast<float>(curHp) / denom, 0.0f, 1.0f);

        if (hpBar->mHasPreviousHpRatio == false)
        {
            hpBar->mHasPreviousHpRatio = true;
            hpBar->mPreviousHpRatio = followRatio;
        }
        else if (followRatio < hpBar->mPreviousHpRatio - 0.001f)
        {
            SpawnHpLossFragments(hpBar, hpBar->mPreviousHpRatio, followRatio);
            hpBar->mPreviousHpRatio = followRatio;
        }
        else
        {
            hpBar->mPreviousHpRatio = followRatio;
        }

        UpdateHpLossFragments(hpBar, dt);
    }
}
