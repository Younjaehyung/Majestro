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
#include "RenderSystem.h"
#include "GameRenderPipeline.h"
#include "PlayerComponent.h" 
#include "TagComponent.h"    
#include "ResourceManager.h"
#include "Texture.h"

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
        if (auto* bt = mWorld->GetComponent<UIButtonComponent>(e)) bt->mEnabled = visible;
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
    if (INPUT.GetKeyDown(eKeyCode::ESC))
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
            SetEntitiesVisible(ctrl->mStateEntities[(size_t)ctrl->mState], true);
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
        break;
    }

    if (prefix == nullptr)
        return;

    // 캐릭터별 배경 텍스처가 등록되어 있을 때만 교체.
    const std::wstring key = std::wstring(L"UI_") + prefix + L"_Paused_Main";
    auto tex = RESOURCEMANAGER.Get<Texture>(key);
    if (tex)
        sprite->mTexture = tex;
}
