#include "pch.h"
#include "BossCutInFeature.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "MathUtils.h"
#include "BossCinematicComponent.h"
#include "EnemyComponent.h"

namespace
{
    inline float EaseOut(float x) { x = MathUtils::Saturate(x); return 1.f - (1.f - x) * (1.f - x); }

    void FillRect(DirectX::SpriteBatch* batch, Texture* white,
        const RECT& rect, float r, float g, float b, float a)
    {
        if (white == nullptr || a <= 0.001f)
            return;
        const DirectX::XMVECTORF32 color{ { { r, g, b, a } } };
        batch->Draw(white->GetSrvGpuHandle(), DirectX::XMUINT2{ 1, 1 }, rect, color);
    }

    void DrawTex(DirectX::SpriteBatch* batch, Texture* tex, const RECT& dst, float alpha)
    {
        if (tex == nullptr || alpha <= 0.001f)
            return;
        const DirectX::XMVECTORF32 col{ { { 1.f, 1.f, 1.f, alpha } } };
        batch->Draw(tex->GetSrvGpuHandle(),
            DirectX::XMUINT2{ static_cast<uint32_t>(tex->GetWidth()),
                              static_cast<uint32_t>(tex->GetHeight()) },
            dst, nullptr, col);
    }
}

void BossCutInFeature::ResolveBossType(BossCutInComponent* cutIn)
{
    if (cutIn->mBossType >= 0 || mWorld == nullptr)
        return;
    if (!mWorld->HasComponentPool<EnemyComponent>())
        return;

	// 보스 타입 해석
    for (Entity e : mWorld->GetEntitiesWithComponent<EnemyComponent>())
    {
        const EnemyComponent* en = mWorld->GetComponent<EnemyComponent>(e);
        if (en == nullptr)
            continue;
        if (en->mEnemyType == EnemyType::Brass || en->mEnemyType == EnemyType::Dragon)
        {
            cutIn->mBossType = static_cast<int>(en->mEnemyType);
            break;
        }
    }
}

void BossCutInFeature::Update(float dt)
{
    if (mWorld == nullptr || !mWorld->HasComponentPool<BossCutInComponent>())
        return;

    BossCutInComponent* cutIn = mWorld->GetSingleton<BossCutInComponent>();
    if (cutIn == nullptr)
        return;

    if (mWorld->HasComponentPool<BossCinematicComponent>())
    {
        const Entity singleton = mWorld->GetSingletonEntity();
        const BossCinematicComponent* seq = mWorld->GetComponent<BossCinematicComponent>(singleton);
        const bool done = (seq != nullptr) && seq->mDone;

        if (done && !mPrevDone)
        {
            cutIn->mActive   = true;
            cutIn->mElapsed  = 0.f;
            cutIn->mBossType = -1;   // 새 컷인마다 보스 재해석
        }
        mPrevDone = done;
    }

    if (!cutIn->mActive)
        return;


    ResolveBossType(cutIn);

    cutIn->mElapsed += dt;
    if (cutIn->mElapsed >= kTotal)
    {
        cutIn->mActive  = false;
        cutIn->mElapsed = 0.f;
    }
}

void BossCutInFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{
    if (mWorld == nullptr || !mWorld->HasComponentPool<BossCutInComponent>())
        return;

    const BossCutInComponent* cutIn = mWorld->GetSingleton<BossCutInComponent>();
    if (cutIn == nullptr || !cutIn->mActive)
        return;

    const float t = cutIn->mElapsed; // 컷인 시작 후 경과

    // 전체 페이드 envelope (빠른 인 + 아웃)
    const float fadeIn  = MathUtils::Saturate(t / 0.12f);
    const float fadeOut = 1.f - MathUtils::Saturate((t - (kInDur + kHoldDur)) / kOutDur);
    const float gate = (std::min)(fadeIn, fadeOut);
    if (gate <= 0.001f)
        return;

    const WindowInfo& window = RENDERMANAGER.GetWindow();
    const float W = static_cast<float>(window.Width);
    const float H = static_cast<float>(window.Height);
    if (W <= 0.f || H <= 0.f)
        return;

    const std::wstring name = BossAssetName(cutIn->mBossType);
    Texture* white = RESOURCEMANAGER.Get<Texture>(L"UI_White").get();
    Texture* back  = RESOURCEMANAGER.Get<Texture>(L"UI_Boss_Cutin_Back").get();
    Texture* band  = RESOURCEMANAGER.Get<Texture>((L"UI_Boss_Cutin_" + name).c_str()).get();
    Texture* img   = RESOURCEMANAGER.Get<Texture>((L"UI_Boss_Cutin_" + name + L"_Img").c_str()).get();

    const RECT fullScreen{ 0, 0, static_cast<LONG>(W), static_cast<LONG>(H) };

    // 오버스캔(슬라이드 시 가장자리 노출 방지)
    const float osx = 0.06f * W;
    const float osy = 0.06f * H;

    // 레이어별 진입 타이밍
    const float tBackIn    = 0.16f;
    const float tBandDelay = 0.05f, tBandIn = 0.20f;
    const float tCharDelay = 0.10f, tCharIn = 0.22f;
    const float tImpact    = tCharDelay + tCharIn;

    // 임팩트 쉐이크(캐릭터 착지 시)
    float sx = 0.f, sy = 0.f;
    {
        const float kShakeDur = 0.26f;
        const float local = t - tImpact;
        if (local >= 0.f && local < kShakeDur)
        {
            const float decay = 1.f - local / kShakeDur;
            const float ph = local * 60.f;
            sx = std::sin(ph)               * (0.016f * W) * decay;
            sy = std::sin(ph * 1.3f + 1.1f) * (0.020f * H) * decay;
        }
    }

    // 퇴장 슬라이드 진행(밴드=왼쪽, 캐릭터=오른쪽으로 빠져나감)
    const float outRaw = MathUtils::Saturate((t - (kInDur + kHoldDur)) / kOutDur);
    const float exit = outRaw * outRaw; // ease-in

    auto layerRect = [&](float slide, float extra) -> RECT {
        return RECT{
            static_cast<LONG>(-osx + slide + extra + sx), static_cast<LONG>(-osy + sy),
            static_cast<LONG>( W + osx + slide + extra + sx), static_cast<LONG>( H + osy + sy) };
    };

    // 배경 디밍
    FillRect(spriteBatch, white, fullScreen, 0.10f, 0.f, 0.f, 0.62f * gate);

    // 배경 잉크 슬래시
    if (back != nullptr)
    {
        const float in = EaseOut(MathUtils::Saturate(t / tBackIn));
        const float slide = (1.f - in) * (-0.30f * W) - exit * (0.10f * W);
        DrawTex(spriteBatch, back, layerRect(slide, 0.f), gate);
    }

    // 이름 밴드
    if (band != nullptr)
    {
        const float in = EaseOut(MathUtils::Saturate((t - tBandDelay) / tBandIn));
        const float slide = (1.f - in) * (-0.20f * W) - exit * (0.18f * W);
        DrawTex(spriteBatch, band, layerRect(slide, 0.f), gate);
    }

    // 보스 캐릭터
    if (img != nullptr)
    {
        const float raw = MathUtils::Saturate((t - tCharDelay) / tCharIn);
        const float in = MathUtils::EaseOutBack(raw);
        const float slide = (1.f - in) * (0.28f * W) + exit * (0.20f * W);
        const float move = 1.f - raw;

        // 잔상
        if (move > 0.01f)
        {
            const int N = 3;
            for (int k = 1; k <= N; ++k)
            {
                const float off = k * 0.05f * W * move;
                const float a = gate * 0.30f * move * (1.f - static_cast<float>(k - 1) / N);
                DrawTex(spriteBatch, img, layerRect(slide, off), a);
            }
        }

        // 본체
        DrawTex(spriteBatch, img, layerRect(slide, 0.f), gate);
    }

    // 임팩트 플래시
    {
        const float local = t - tImpact;
        float flash = 0.f;
        if (local >= 0.f)
        {
            const float f1 = 1.f - MathUtils::Saturate(local / 0.06f);
            const float f2 = local > 0.06f ? (1.f - MathUtils::Saturate((local - 0.06f) / 0.12f)) * 0.5f : 0.f;
            flash = (std::max)(f1, f2);
        }
        FillRect(spriteBatch, white, fullScreen, 1.f, 1.f, 1.f, flash * gate * 0.85f);
    }
}
