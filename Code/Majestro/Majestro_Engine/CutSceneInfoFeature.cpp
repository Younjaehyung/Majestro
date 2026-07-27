#include "pch.h"
#include "CutSceneInfoFeature.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "MathUtils.h"
#include "IntroSequenceComponent.h"

namespace
{

    // Ui_CutScene_Info_Sheet 아틀라스 구성 — 2048x2048 을 1024x512 셀 2열 x 4행으로 나눈다.
    constexpr int kAtlasW    = 2048;
    constexpr int kAtlasH    = 2048;
    constexpr int kCellW     = 1024;
    constexpr int kCellH     = 512;

    // 셀 좌표 (col, row)
    constexpr int kBackdropCol = 0, kBackdropRow = 0;  // 검정 잉크 wash
    constexpr int kLineCol     = 1, kLineRow     = 0;  // 흰 선 (원 + 십자 + 가로 룰)

    RECT CellRect(int col, int row)
    {
        const LONG x = static_cast<LONG>(col) * kCellW;
        const LONG y = static_cast<LONG>(row) * kCellH;
        return RECT{ x, y, x + kCellW, y + kCellH };
    }

    // 씬별 맵 소개 텍스트 셀.
    // 곡 구성 순서(Intro→Chorus→Bridge→Outro)가 스테이지 순서와 일치한다.
    //   FirstGame  : Intro     / LOOK BACK TOWN
    //   SecondGame : Chorus    / SANDMAN DESERT
    //   ThirdGame  : Bridge    / BASCKET CASTLE
    //   FourthGame : Outro     / HIGHWAY TO HELL
    //   Plaza      : Rehearsal / SKY LOFT       (광장은 인트로 시퀀스가 없어 현재 미사용)
    bool TextCellForScene(SceneId scene, RECT& outRect)
    {
        switch (scene)
        {
        case SceneId::FirstGame:  outRect = CellRect(1, 1); return true;
        case SceneId::SecondGame: outRect = CellRect(0, 2); return true;
        case SceneId::ThirdGame:  outRect = CellRect(1, 2); return true;
        case SceneId::FourthGame: outRect = CellRect(0, 3); return true;
        case SceneId::Plaza:      outRect = CellRect(0, 1); return true;
        default:                  return false;
        }
    }

    void DrawCell(DirectX::SpriteBatch* batch, Texture* atlas,
        const RECT& src, const RECT& dst, float alpha)
    {
        if (atlas == nullptr || alpha <= 0.001f)
            return;

        const DirectX::XMVECTORF32 color{ { { 1.f, 1.f, 1.f, alpha } } };
        batch->Draw(atlas->GetSrvGpuHandle(),
            DirectX::XMUINT2{ static_cast<uint32_t>(kAtlasW), static_cast<uint32_t>(kAtlasH) },
            dst, &src, color);
    }
}

void CutSceneInfoFeature::SpriteRender(DirectX::SpriteBatch* spriteBatch)
{
    if (mWorld == nullptr || !mWorld->HasComponentPool<IntroSequenceComponent>())
        return;

    // 씬 진입 시네마틱 카메라가 실제로 재생 중일 때만 노출한다.
    const IntroSequenceComponent* seq = mWorld->GetSingleton<IntroSequenceComponent>();
    if (seq == nullptr || !seq->mPlaying)
        return;

    RECT textSrc{};
    if (!TextCellForScene(mWorld->GetSceneId(), textSrc))
        return;

    Texture* atlas = RESOURCEMANAGER.Get<Texture>(L"UI_CutScene_Info_Sheet").get();
    if (atlas == nullptr)
        return;

    // ---- 등장/퇴장 envelope ----
    const float t   = seq->mElapsed;
    const float dur = seq->Duration();

    const float fadeIn  = MathUtils::Saturate(t / kFadeInDur);
    const float fadeOut = (dur > kFadeOutDur)
        ? MathUtils::Saturate((dur - t) / kFadeOutDur)
        : 1.f;
    const float gate = (std::min)(fadeIn, fadeOut);
    if (gate <= 0.001f)
        return;

    // SpriteBatch 가 실제로 그리는 좌표계(= UIRenderSystem 이 SetViewport 한 뷰포트)를 기준으로 삼는다.
    // 전체화면/해상도가 바뀌어도 그리는 공간과 앵커 기준이 어긋나지 않는다.
    const D3D12_VIEWPORT& viewport = RENDERMANAGER.GetViewPort();
    const float W = viewport.Width;
    const float H = viewport.Height;
    if (W <= 0.f || H <= 0.f)
        return;

    // ---- 좌하단 밀착 배치 (원본 1024:512 비율 유지) ----
    // 셀 내부 투명 여백을 상쇄해 그림 자체가 좌하단 모서리에 딱 맞도록 카드 rect 를 잡는다.
    // 남는 여백은 화면 밖으로 밀려나며 전부 투명이라 잘려도 보이는 변화가 없다.
    const float cardH = kCardHeightRatio * H;
    const float cardW = cardH * (static_cast<float>(kCellW) / static_cast<float>(kCellH));
    const float cardX = -kContentLeftRatio * cardW;
    const float cardY = H - kContentBottomRatio * cardH;

    // 좌측에서 살짝 밀려 들어오는 연출
    const float slide = (1.f - MathUtils::EaseOut(MathUtils::Saturate(t / kSlideInDur))) * (-0.06f * cardW);

    const RECT dst{
        static_cast<LONG>(cardX + slide),
        static_cast<LONG>(cardY),
        static_cast<LONG>(cardX + slide + cardW),
        static_cast<LONG>(cardY + cardH) };

    // 세 레이어는 같은 1024x512 캔버스에 정합되어 있으므로 같은 dst 에 순서대로 겹쳐 그린다.
    DrawCell(spriteBatch, atlas, CellRect(kBackdropCol, kBackdropRow), dst, gate);  // 검정 wash
    DrawCell(spriteBatch, atlas, CellRect(kLineCol, kLineRow),         dst, gate);  // 흰 선
    DrawCell(spriteBatch, atlas, textSrc,                              dst, gate);  // 맵 텍스트
}
