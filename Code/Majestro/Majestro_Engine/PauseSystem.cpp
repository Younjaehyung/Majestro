#include "pch.h"
#include "PauseSystem.h"
#include "Engine.h"
#include "InputManager.h"
#include "PauseMenuController.h"
#include "PlayerInputSystem.h"
#include "UISpriteComponent.h"
#include "UIVfxComponent.h"
#include "UITextComponent.h"
#include "UIButtonComponent.h"
#include "UITransformComponent.h"
#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "PlayerComponent.h" 
#include "TagComponent.h"    
#include "ResourceManager.h"
#include "Texture.h"
#include "MathUtils.h"
#include "NpcComponent.h"

PauseSystem::PauseSystem(World* world) : System(world)
{
    mPhase = SysPhase::Pre;
}

std::vector<std::type_index> PauseSystem::Before() const
{
    // 같은 프레임에 입력이 막히도록 PlayerInputSystem 보다 먼저 실행
    return { typeid(PlayerInputSystem) };
}

void PauseSystem::SetEntitiesVisible(const std::vector<Entity>& es, bool visible)
{
    for (Entity e : es)
    {
        if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e)) sp->mVisible = visible;
        if (auto* vf = mWorld->GetComponent<UIVfxComponent>(e))    vf->mVisible = visible;
        if (auto* tx = mWorld->GetComponent<UITextComponent>(e))   tx->mVisible = visible;
        if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e))
        {
            // Pause 하위 화면으로 전환할 때 루트 버튼의 호버 선이 남지 않도록 정리한다.
            if (!visible && bt->mHovered && bt->mOnHoverExit)
                bt->mOnHoverExit();
            if (!visible)
            {
                bt->mHovered = false;
                bt->mPressed = false;
            }
            bt->mEnabled = visible;
        }
    }
}

void PauseSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<PauseMenuController>()) return;

    auto entities = mWorld->GetEntitiesWithComponent<PauseMenuController>();
    if (entities.empty()) return;

    auto* ctrl = mWorld->GetComponent<PauseMenuController>(entities[0]);
    if (!ctrl) return;

    // ESC 키: 일시정지 토글. 어느 서브 상태에 있든 닫으면 바로 게임으로 복귀.
    // 단, NPC 대화/레벨 선택 UI가 열려 있으면 해당 UI의 닫기가 우선 (NpcInteractionSystem 처리)
    const DialogueStateComponent* dialogue = mWorld->GetSingleton<DialogueStateComponent>();
    const bool dialogueBlocking = dialogue != nullptr && (dialogue->mActive || dialogue->mLevelSelectActive);
    if (INPUT.GetKeyDown(eKeyCode::ESC) && !dialogueBlocking)
    {
        ctrl->mPaused = !ctrl->mPaused;
        ctrl->Request(ctrl->mPaused ? PauseMenuState::Root : PauseMenuState::Hidden);
    }


    // ESC 키뿐 아니라 Resume 버튼 클릭으로 mPaused 가 풀린 경우도 같이 처리
    if (ctrl->mPaused != mPrevPaused)
    {
        if (ctrl->mPaused)
        {
            mSavedMouseLook = INPUT.IsMouseLookActive(); // 진입 전 상태 저장
            INPUT.SetForceMouseLook(false);              // 버튼 클릭 가능
            SetPauseBlur(true);                          // 게임 화면 전체 blur
            ApplyBackgroundTexture(ctrl);                // 로컬 캐릭터별 배경 레이어
        }
        else
        {
            INPUT.SetForceMouseLook(mSavedMouseLook);     // 복귀 시 원래 상태로
            SetPauseBlur(false);                          // blur 해제
        }
        mPrevPaused = ctrl->mPaused;
    }

    // 상태 전환 적용
    if (ctrl->mHasPending)
    {
        const PauseMenuState prev = ctrl->mState;
        ctrl->mState      = ctrl->mPendingState;
        ctrl->mHasPending = false;

        if (prev != ctrl->mState)
        {
            SetEntitiesVisible(ctrl->mStateEntities[(size_t)prev], false);
            ApplyBackgroundTexture(ctrl);
            SetEntitiesVisible(ctrl->mStateEntities[(size_t)ctrl->mState], true);
        }
    }

    if (ctrl->mPaused)
    {
        // 배경 등장 연출 : 알파 0 -> 목표, 스케일 mBgStartScale -> 1.0 (SmoothStep 보간)
        if (ctrl->mBgFadeT < 1.f && ctrl->mBackgroundEntity != NULL_ENTITY)
        {
            ctrl->mBgFadeT = (std::min)(1.f, ctrl->mBgFadeT + dt / ctrl->mBgFadeDuration);
            const float s = SmoothStep01(ctrl->mBgFadeT);

            if (auto* sp = mWorld->GetComponent<UISpriteComponent>(ctrl->mBackgroundEntity))
                sp->mColorTint.w = ctrl->mBgTargetAlpha * s;

            if (auto* tr = mWorld->GetComponent<UITransformComponent>(ctrl->mBackgroundEntity))
            {
                const float scale = ctrl->mBgStartScale + (1.0f - ctrl->mBgStartScale) * s;
                tr->mScale = Vec2(scale, scale);
            }
        }

        // 두 장을 한 세트로 왼쪽 이동시키고 첫 장이 화면 밖으로 나가면
        // 정확히 한 장 너비만큼 되돌려 끊김 없는 반복 띠를 유지한다.
        ctrl->mWordBandOffset -= ctrl->mWordBandScrollSpeed * dt;
        while (ctrl->mWordBandOffset <= -ctrl->mWordBandTileWidth)
            ctrl->mWordBandOffset += ctrl->mWordBandTileWidth;

        const float firstX = ctrl->mWordBandOffset;
        const float secondX = firstX + ctrl->mWordBandTileWidth;
        const Vec2 positions[4] = {
            Vec2(firstX, ctrl->mWordBandTopY),
            Vec2(secondX, ctrl->mWordBandTopY),
            Vec2(firstX, ctrl->mWordBandBottomY),
            Vec2(secondX, ctrl->mWordBandBottomY)
        };

        for (size_t i = 0; i < ctrl->mWordBandEntities.size(); ++i)
        {
            if (UITransformComponent* transform =
                mWorld->GetComponent<UITransformComponent>(ctrl->mWordBandEntities[i]))
            {
                transform->mPosition = positions[i];
            }
        }
    }
}

void PauseSystem::SetPauseBlur(bool on)
{
    auto* rs = mWorld->GetSystemManager()->GetSystem<RenderSystem>();
    if (!rs) return;

    auto pipeline = std::static_pointer_cast<GameRenderPipeline>(rs->GetPipeline());
    if (!pipeline) return;

    pipeline->SetBlur(on);
}

void PauseSystem::ApplyBackgroundTexture(PauseMenuController* ctrl)
{
    if (!ctrl || ctrl->mBackgroundEntity == NULL_ENTITY)
        return;

    auto* sprite = mWorld->GetComponent<UISpriteComponent>(ctrl->mBackgroundEntity);
    if (!sprite)
        return;

    // 로컬 플레이어의 캐릭터 타입 조회
    const wchar_t* prefix = nullptr;
    PlayerType localPlayerType = PlayerType::Count;
    for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
    {
        if (!mWorld->HasComponent<LocalPlayerComponent>(e))
            continue;

        auto* mp = mWorld->GetComponent<MainPlayerComponent>(e);
        if (!mp)
            break;

        switch (mp->mPlayerType)
        {
        case PlayerType::Rudwig:  prefix = L"Rudwig";  break;
        case PlayerType::Ibanix:  prefix = L"Ibanix";  break;
        case PlayerType::Fanthor: prefix = L"Fanthor"; break;
        default:                  prefix = nullptr;    break;
        }
        localPlayerType = mp->mPlayerType;
        break;
    }

    if (prefix == nullptr)
        return;

    // 캐릭터별 배경 텍스처가 등록되어 있을 때만 교체.
    const wchar_t* screenSuffix = L"Main";
    switch (ctrl->mState)
    {
    case PauseMenuState::Manual:
        screenSuffix = L"Manual";
        break;
    case PauseMenuState::SettingGraphics:
    case PauseMenuState::SettingSound:
        screenSuffix = L"Setting";
        break;
    case PauseMenuState::ConfirmDisconnect:
        screenSuffix = L"Quit";
        break;
    default:
        screenSuffix = L"Main";
        break;
    }

    const std::wstring key =
        std::wstring(L"UI_") + prefix + L"_Paused_" + screenSuffix;
    auto tex = RESOURCEMANAGER.Get<Texture>(key);
    if (tex)
        sprite->mTexture = tex;

    // 캐릭터 일러스트가 차지하지 않는 검은 영역에 메뉴를 배치한다.
    const bool useRightMenu = localPlayerType == PlayerType::Fanthor;
    const float menuLeftX = useRightMenu ? 200.f : -1000.f;
    const float menuY[4] = { -220.f, 10.f, 240.f, 470.f };

    RECT hoverSource = { 0, 366, 768, 405 };
    if (localPlayerType == PlayerType::Rudwig)
        hoverSource = RECT{ 768, 366, 1536, 405 };
    else if (localPlayerType == PlayerType::Ibanix)
        hoverSource = RECT{ 0, 526, 768, 565 };

    // 아틀라스의 첫 행은 Ibanix, 둘째 행은 Ludwig, 셋째 행은 Fanthor 색상이다.
    RECT wordBandSource = { 0, 0, 2048, 128 };
    if (localPlayerType == PlayerType::Rudwig)
        wordBandSource = RECT{ 0, 128, 2048, 256 };
    else if (localPlayerType == PlayerType::Fanthor)
        wordBandSource = RECT{ 0, 256, 2048, 384 };

    for (Entity bandEntity : ctrl->mWordBandEntities)
    {
        if (UISpriteComponent* bandSprite =
            mWorld->GetComponent<UISpriteComponent>(bandEntity))
        {
            bandSprite->SetSourceRect(
                static_cast<float>(wordBandSource.left),
                static_cast<float>(wordBandSource.top),
                static_cast<float>(wordBandSource.right - wordBandSource.left),
                static_cast<float>(wordBandSource.bottom - wordBandSource.top));
        }
    }

    for (size_t i = 0; i < ctrl->mRootButtons.size(); ++i)
    {
        if (auto* tr = mWorld->GetComponent<UITransformComponent>(ctrl->mRootButtons[i]))
            tr->mPosition = Vec2(menuLeftX, menuY[i]);

        if (auto* lineTr = mWorld->GetComponent<UITransformComponent>(ctrl->mRootHoverLines[i]))
            lineTr->mPosition = Vec2(menuLeftX - 90.f, menuY[i] + 120.f);

        if (auto* lineSprite = mWorld->GetComponent<UISpriteComponent>(ctrl->mRootHoverLines[i]))
        {
            lineSprite->SetSourceRect(
                static_cast<float>(hoverSource.left),
                static_cast<float>(hoverSource.top),
                static_cast<float>(hoverSource.right - hoverSource.left),
                static_cast<float>(hoverSource.bottom - hoverSource.top));
            lineSprite->mVisible = false;
        }
    }

    // 배경 텍스처 교체 직후 등장 연출(페이드인 + 줌인)을 처음부터 재생한다.
    // Pause 진입과 하위 화면 전환 모두 이 함수를 거치므로 메인메뉴처럼 매번 "촥" 등장한다.
    ctrl->mBgFadeT       = 0.f;
    sprite->mColorTint.w = 0.f;
    if (auto* bgTr = mWorld->GetComponent<UITransformComponent>(ctrl->mBackgroundEntity))
        bgTr->mScale = Vec2(ctrl->mBgStartScale, ctrl->mBgStartScale);
}
