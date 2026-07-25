#include "pch.h"
#include "UILevelSelectFeature.h"
#include "NpcComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIButtonFactory.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Engine.h"
#include "GameEvents.h"
#include "CameraView.h"

namespace
{
	constexpr const wchar_t* kSheetKey = L"UI_LevelMan_Talk_Level";

	constexpr int32 kStageCount = UILevelSelectFeature::kStageCount;

	constexpr RECT kCardRects[kStageCount] =
	{
		{ 0,    108,  1005, 888 },   // STAGE 01 INTRO 카드
		{ 1030, 108,  2044, 888 },   // STAGE 02 CHROUS 카드
		{ 0,    1132, 1005, 1912 },  // STAGE 03 BRIDGE 카드
		{ 0,    2156, 1020, 2936 },  // STAGE 04 OUTRO 카드
	};
	constexpr RECT kStageBtnRects[kStageCount] =	// 슬래시 원 포함 192px 균일 밴드
	{
		{ 1005, 1056, 1536, 1248 },
		{ 1005, 1312, 1536, 1504 },
		{ 1005, 1568, 1536, 1760 },
		{ 1005, 2080, 1536, 2272 },	// 시트상 GO! 아래에 배치된 밴드
	};
	constexpr RECT kGoBtnRect = { 1120, 1830, 1540, 2048 };

	// 스테이지 버튼 세로 배치
	constexpr float kStageBtnWidth   = 576.f;	// 소스 531x192 비율 유지
	constexpr float kStageBtnHeight  = 208.f;
	constexpr float kStageBtnSpacing = 260.f;
	constexpr float kStageBtnCenterY = -30.f;	// 버튼 스택의 세로 중심
	constexpr float kStageBtnX       = 560.f;

	// 선택/비선택 스테이지 버튼 틴트
	const Vec4 kSelectedTint   = { 1.f, 1.f, 1.f, 1.f };
	const Vec4 kUnselectedTint = { 0.5f, 0.5f, 0.5f, 0.9f };

	const SceneId kStageScenes[kStageCount] =
	{
		SceneId::FirstGame, SceneId::SecondGame, SceneId::ThirdGame, SceneId::FourthGame
	};

	Entity CreateSheetSprite(World* world, const Vec2& pos, const Vec2& size, uint8 layer,
		const std::wstring& texKey, const RECT* srcRect)
	{
		Entity e = world->CreateEntity();
		auto& tr = world->AddComponent<UITransformComponent>(e);
		tr.mAnchor = Anchor::Center;
		tr.mPosition = pos;
		tr.mSize = size;
		tr.mPivot = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = layer;

		auto& sp = world->AddComponent<UISpriteComponent>(e,
			RESOURCEMANAGER.Get<Texture>(texKey));
		if (srcRect)
		{
			sp.SetSourceRect(static_cast<float>(srcRect->left),
				static_cast<float>(srcRect->top),
				static_cast<float>(srcRect->right - srcRect->left),
				static_cast<float>(srcRect->bottom - srcRect->top));
		}
		sp.mVisible = false;
		return e;
	}
}

void UILevelSelectFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);

	// 풀스크린 오버레이 (반투명 딤 + LEVEL 타이틀)
	mBackground = CreateSheetSprite(world, Vec2(0.f, 0.f), Vec2(2560.f, 1440.f), 4, L"UI_LevelMan_Talk_0", nullptr);
	
	if (UITransformComponent* backgroundTransform = world->GetComponent<UITransformComponent>(mBackground))
	{
		// 전체화면
		backgroundTransform->UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	}

	// 선택된 스테이지 정보 카드 (좌측)
	mStageCard = CreateSheetSprite(world, Vec2(-620.f, 20.f), Vec2(1010.f, 780.f),
		5, kSheetKey, &kCardRects[0]);

	// STAGE 01~04 버튼 (우측 세로 배치)
	for (int32 i = 0; i < kStageCount; ++i)
	{
		const float offsetY = (static_cast<float>(i) - (kStageCount - 1) * 0.5f) * kStageBtnSpacing;

		UIButtonDesc desc;
		desc.anchor   = Anchor::Center;
		desc.position = Vec2(kStageBtnX, kStageBtnCenterY + offsetY);
		desc.size     = Vec2(kStageBtnWidth, kStageBtnHeight);
		desc.layer    = 6;
		desc.visual   = UIButtonVisual::Texture;
		desc.resKey   = kSheetKey;
		desc.sourceRect = kStageBtnRects[i];
		desc.normalColor  = kUnselectedTint;
		desc.hoveredColor = kSelectedTint;
		desc.onClick = [world, i]()
		{
			if (DialogueStateComponent* state = world->GetSingleton<DialogueStateComponent>())
				state->mSelectedStage = i;
		};
		mStageButtons[i] = CreateUIButton(world, desc);
	}

	// GO! 버튼 — 선택한 스테이지로 진입 요청
	{
		UIButtonDesc desc;
		desc.anchor   = Anchor::Center;
		desc.position = Vec2(620.f, 580.f);	// 스테이지 버튼 4개 스택 아래
		desc.size     = Vec2(300.f, 156.f);	// 소스 420x218 비율 유지
		desc.layer    = 6;
		desc.visual   = UIButtonVisual::Texture;
		desc.resKey   = kSheetKey;
		desc.sourceRect = kGoBtnRect;
		desc.hoveredScale = 1.1f;
		desc.onClick = [world]()
		{
			DialogueStateComponent* state = world->GetSingleton<DialogueStateComponent>();
			if (!state)
				return;

			const int32 stage = std::clamp(state->mSelectedStage, 0, kStageCount - 1);
			Cinematic::RequestPlazaLevelEnter(world, kStageScenes[stage]);
			state->mLevelSelectActive = false; // 커서 복원은 NpcInteractionSystem이 처리
		};
		mGoButton = CreateUIButton(world, desc);
	}

	// 레벨 선택 UI는 대화창과 같은 Dialogue 그룹으로 렌더링
	world->AddComponent<UIRenderGroupComponent>(mBackground, UIRenderGroup::Dialogue);
	world->AddComponent<UIRenderGroupComponent>(mStageCard, UIRenderGroup::Dialogue);
	for (Entity e : mStageButtons)
		world->AddComponent<UIRenderGroupComponent>(e, UIRenderGroup::Dialogue);
	world->AddComponent<UIRenderGroupComponent>(mGoButton, UIRenderGroup::Dialogue);

	SetVisible(false);
}

void UILevelSelectFeature::Update(float dt)
{
	DialogueStateComponent* state = mWorld->GetSingleton<DialogueStateComponent>();
	if (!state)
		return;

	const bool active = state->mLevelSelectActive;
	if (active != mVisible)
	{
		SetVisible(active);
		mVisible = active;
		mAppliedStage = -1; // 열릴 때 카드/틴트 강제 갱신
	}

	if (active && state->mSelectedStage != mAppliedStage)
	{
		ApplySelection(std::clamp(state->mSelectedStage, 0, kStageCount - 1));
		mAppliedStage = state->mSelectedStage;
	}
}

void UILevelSelectFeature::SetVisible(bool visible)
{
	auto setSprite = [this, visible](Entity e)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mVisible = visible;
	};

	setSprite(mBackground);
	setSprite(mStageCard);
	setSprite(mGoButton);
	for (Entity e : mStageButtons)
		setSprite(e);

	// 숨겨진 동안 히트테스트/클릭이 발화하지 않도록 버튼 비활성화
	auto setEnabled = [this, visible](Entity e)
	{
		if (UIButtonComponent* btn = mWorld->GetComponent<UIButtonComponent>(e))
			btn->mEnabled = visible;
	};
	setEnabled(mGoButton);
	for (Entity e : mStageButtons)
		setEnabled(e);
}

void UILevelSelectFeature::ApplySelection(int32 stage)
{
	// 선택된 스테이지의 정보 카드로 교체
	if (UISpriteComponent* card = mWorld->GetComponent<UISpriteComponent>(mStageCard))
	{
		const RECT& r = kCardRects[stage];
		card->SetSourceRect(static_cast<float>(r.left), static_cast<float>(r.top),
			static_cast<float>(r.right - r.left), static_cast<float>(r.bottom - r.top));
	}

	// 선택된 버튼만 밝게 표시
	for (int32 i = 0; i < kStageCount; ++i)
	{
		UIButtonComponent* btn = mWorld->GetComponent<UIButtonComponent>(mStageButtons[i]);
		UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(mStageButtons[i]);
		if (!btn)
			continue;

		const Vec4& tint = (i == stage) ? kSelectedTint : kUnselectedTint;
		btn->mNormalColor = tint;
		// 호버/프레스 중이 아니면 즉시 반영 (UIButtonSystem은 상태 전환 시에만 갱신)
		if (sp && btn->mPrevState == ButtonVisualState::Normal)
			sp->mColorTint = tint;
	}
}
