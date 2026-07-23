#include "pch.h"
#include "UltimateCutInFeature.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "MathUtils.h"
#include "PlayerComponent.h"
#include "TagComponent.h"

namespace
{
    inline float EaseOut(float x) { x = MathUtils::Saturate(x); return 1.f - (1.f - x) * (1.f - x); }

    const wchar_t* CharacterName(uint8_t playerType)
    {
        switch (playerType)
        {
        case 0:  return L"Rudwig";
        case 1:  return L"Ibanix";
        case 2:  return L"Fanthor";
        default: return L"Rudwig";
        }
    }


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

void UltimateCutInFeature::Update(float dt)
{

    bool    casting    = false;
    uint8_t playerType = 0;
    if (mWorld != nullptr)
    {
        for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
        {
            if (!mWorld->HasComponent<LocalPlayerComponent>(e))
                continue;

            const MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(e);
            if (player != nullptr &&
                player->mUpperState == static_cast<int>(ReplicatedActionState::UltimateIntro))
            {
                casting    = true;
                playerType = static_cast<uint8_t>(player->mPlayerType);
            }
            break; // 로컬 플레이어는 하나뿐
        }
    }


    if (casting && !mCasting)
        mElapsed = 0.f;

    if (casting)
        mPlayerType = playerType;
    mCasting = casting;

    const float target = casting ? 1.f : 0.f;
    const float speed  = casting ? kFadeInSpeed : kFadeOutSpeed;
    if (mFadeGate < target)
        mFadeGate = (std::min)(target, mFadeGate + speed * dt);
    else if (mFadeGate > target)
        mFadeGate = (std::max)(target, mFadeGate - speed * dt);

    if (mFadeGate > 0.001f)
        mElapsed += dt;
    else
        mElapsed = 0.f;
}

void UltimateCutInFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{
    if (mFadeGate <= 0.001f) // 그릴 필요가 있는지
        return;

    const float gate = mFadeGate; // 전체 페이드 0~1
    const float t    = mElapsed;  // 컷인 시작 후 경과

    const WindowInfo& window = RENDERMANAGER.GetWindow();
    const float W = static_cast<float>(window.Width);
    const float H = static_cast<float>(window.Height);
    if (W <= 0.f || H <= 0.f)
        return;


    Texture* white = RESOURCEMANAGER.Get<Texture>(L"UI_White").get();

    const std::wstring name = CharacterName(mPlayerType);
    Texture* bg  = RESOURCEMANAGER.Get<Texture>((L"UI_" + name + L"_Cutin_Back").c_str()).get();
    Texture* por = RESOURCEMANAGER.Get<Texture>((L"UI_" + name + L"_Cutin").c_str()).get();

    const RECT fullScreen{ 0, 0, static_cast<LONG>(W), static_cast<LONG>(H) };

    // 타이밍
    const float kBgSlide   = 0.22f;   // 배경 진입
    const float kCharSlide = 0.18f;   // 캐릭터 진입
    const float tImpact    = kCharSlide;

    // 오버스캔
    const float osx = 0.05f * W;
    const float osy = 0.05f * H;

    // 임팩트 쉐이크
    float sx = 0.f, sy = 0.f;
    {
        const float kShakeDur = 0.22f;
        const float local = t - tImpact;
        if (local >= 0.f && local < kShakeDur)
        {
            const float decay = 1.f - local / kShakeDur;
            const float ph = local * 58.f;
            sx = std::sin(ph)               * (0.012f * W) * decay;
            sy = std::sin(ph * 1.3f + 1.1f) * (0.016f * H) * decay;
        }
    }

    // 배경 디밍
    FillRect(spriteBatch, white, fullScreen, 0.f, 0.f, 0.f, 0.55f * gate);

    // 배경 스피드라인/슬래시
    if (bg != nullptr)
    {
        const float in = EaseOut(t / kBgSlide);
        const float slide = (1.f - in) * (-0.18f * W);
        const RECT dst{
            static_cast<LONG>(-osx + slide + sx), static_cast<LONG>(-osy + sy),
            static_cast<LONG>( W + osx + slide + sx), static_cast<LONG>( H + osy + sy) };
        DrawTex(spriteBatch, bg, dst, gate);
    }

    // 캐릭터 컷인
    if (por != nullptr)
    {
        const float inRaw = t / kCharSlide;
        const float in = MathUtils::EaseOutBack(MathUtils::Saturate(inRaw));
        const float slide = (1.f - in) * (0.18f * W);
        const float move = 1.f - MathUtils::Saturate(inRaw);


        auto charRect = [&](float extra) -> RECT {
            return RECT{
                static_cast<LONG>(-osx + slide + extra + sx), static_cast<LONG>(-osy + sy),
                static_cast<LONG>( W + osx + slide + extra + sx), static_cast<LONG>( H + osy + sy) };
        };

        // 모션 스트릭
        if (move > 0.01f)
        {
            const int N = 3;
            for (int k = 1; k <= N; ++k)
            {
                const float off = k * 0.05f * W * move;
                const float a = gate * 0.28f * move * (1.f - static_cast<float>(k - 1) / N);
                DrawTex(spriteBatch, por, charRect(off), a);
            }
        }

        // 본체
        DrawTex(spriteBatch, por, charRect(0.f), gate);
    }

    // 임팩트 플래시
    {
        const float local = t - tImpact;
        float flash = 0.f;
        if (local >= 0.f)
        {
            const float f1 = 1.f - MathUtils::Saturate(local / 0.07f);
            const float f2 = local > 0.07f ? (1.f - MathUtils::Saturate((local - 0.07f) / 0.12f)) * 0.5f : 0.f;
            flash = (std::max)(f1, f2);
        }
        FillRect(spriteBatch, white, fullScreen, 1.f, 1.f, 1.f, flash * gate * 0.85f);
    }
}
