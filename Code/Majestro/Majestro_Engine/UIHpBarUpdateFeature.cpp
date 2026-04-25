#include "pch.h"
#include "UIHpBarUpdateFeature.h"

#include "Engine.h"
#include "HealthComponent.h"
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

    // 후속 패스에 영향 가지 않게 GlobalParams 일부 원복
    const uint32 zero = 0;
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 0);  // BaseInstanceID
    GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &zero, 2);  // casdcae (역할 플래그)
}

void UIHpBarUpdateFeature::DrawHpBar(UIHpBarComponent* hpBar, Entity owner)
{
    // 모드별 앵커 결정
    // 두 모드 모두 셰이더 GlobalParams.HpBarAnchorWorld 의 (x,y,z) 슬롯을 재활용:
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

    // per-bar GlobalParams 구성
    GlobalParamsLayout gp{};
    gp.BaseInstanceID = 0;
    gp.etc = hpBar->mIsScreenSpace ? 1u : 0u; // bit0: 1=HUD(screen-space, no occlusion)
    gp.casdcae = 0;            // 0 = 배경 (DrawHpBar 시작 단계)
    gp.PassCustomIndex = 0;

    gp.HpBarAnchorWorldX = anchorXYZ.x;
    gp.HpBarAnchorWorldY = anchorXYZ.y;
    gp.HpBarAnchorWorldZ = anchorXYZ.z;

    gp.HpBarSizePxX = hpBar->mMaxWidth;
    gp.HpBarSizePxY = hpBar->mHeight;

    // 바 좌상단을 앵커 기준 (-w/2, 0) 위치에 두는 피벗 (수평 중앙, 수직 위 정렬)
    gp.HpBarPivotPxX = -hpBar->mMaxWidth * 0.5f;
    gp.HpBarPivotPxY = 0.f;

    gp.HpBarFollowRatio = std::clamp(hpBar->mPreviousHpRatio, 0.f, 1.f);
    gp.HpBarBgTexIdx = (bgTex != nullptr) ? bgTex->GetImageIndex() : 0u;
    gp.HpBarFillTexIdx = (fillTex != nullptr) ? fillTex->GetImageIndex() : 0u;
    gp.HpBarHitTexIdx = (hitTex != nullptr) ? hitTex->GetImageIndex() : 0u;

    // packed: cols(0..7) | rows(8..15) | frameCount(16..31)
    const uint32 cols = static_cast<uint32>((std::max)(1, hpBar->mHitEffectCols)) & 0xFFu;
    const uint32 rows = static_cast<uint32>((std::max)(1, hpBar->mHitEffectRows)) & 0xFFu;
    const uint32 frameCount = static_cast<uint32>((std::max)(1, hpBar->mHitEffectFrameCount)) & 0xFFFFu;
    gp.HpBarHitConfig = cols | (rows << 8u) | (frameCount << 16u);

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
        quadMesh->Render(1, 0, 0, 0); // 배경 (casdcae=0)

        const uint32 role = 1;
        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 1, &role, 2); // 채움 (casdcae=1)
        quadMesh->Render(1, 0, 0, 0);
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
        for (const UIHpLossFragment& mFragment : hpBar->mLossFragments)
        {
            if (mFragment.LifeTime <= 0.f)
                continue;
            const float ageRatio = std::clamp(mFragment.Age / mFragment.LifeTime, 0.f, 1.f);
            const float alpha = 1.f - ageRatio;
            if (alpha <= 0.01f)
                continue;

            UIInstanceData d{};
            d.Position = mFragment.HitAnchorPx + hpBar->mHitEffectOffsetPx; // 경계점 + 시각 보정
            d.Size = hpBar->mHitEffectSizePx;      // 화면 픽셀 크기
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
    // HP 바의 왼쪽 끝이 0, 오른쪽 끝이 mMaxWidth 라고 생각할 때, 잃은 HP 영역의 시작/끝 위치
    const float lostLocalStart = hpBar->mMaxWidth * clampedNew;
    const float lostLocalEnd = hpBar->mMaxWidth * clampedOld;
    const float lostWidth = lostLocalEnd - lostLocalStart; 
	if (lostWidth <= 1.f || hpBar->mHeight <= 0.f)  // 너무 작은 피해는 파편 생략
        return;

    // 앵커 기준 오프셋 (바 좌상단 = (-mMaxWidth/2, 0))
    const float x0 = -hpBar->mMaxWidth * 0.5f + lostLocalStart;
    const float x1 = -hpBar->mMaxWidth * 0.5f + lostLocalEnd;
    const float y0 = 0.f;
    const float y1 = hpBar->mHeight;

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
            const float jx = edgeX ? 0.f : RandomRange(-jitterScaleX, jitterScaleX);
            const float jy = edgeY ? 0.f : RandomRange(-jitterScaleY, jitterScaleY);
            gridAt(i, j) = Vec2(baseX + jx, baseY + jy);
        }
    }

    mFragment.Triangles.clear();
    mFragment.Triangles.reserve(static_cast<size_t>(cols) * rows * 2);
    mFragment.LifeTime = hpBar->mFragmentLifeTime;
    mFragment.Age = 0.f;
    mFragment.HitAnchorPx = Vec2(x0, hpBar->mHeight * 0.5f);
   

    for (int j = 0; j < rows; ++j)
    {
        for (int i = 0; i < cols; ++i)
        {
            const Vec2 tl = gridAt(i, j);
            const Vec2 tr = gridAt(i + 1, j);
            const Vec2 br = gridAt(i + 1, j + 1);
            const Vec2 bl = gridAt(i, j + 1);

            if (RandomRange(0.f, 1.f) < 0.5f)
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

        HealthComponent* followHealth = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);
        if (followHealth == nullptr)
            continue;

        const float followRatio = std::clamp(static_cast<float>(followHealth->mCurrentHp) /static_cast<float>((std::max)(1, followHealth->mMaxHp)),  0.0f, 1.0f);

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
