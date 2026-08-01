#include "pch.h"
#include "UIRhythmSelectFeature.h"
#include "NpcComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIButtonFactory.h"
#include "CircularVisualizerComponent.h"
#include "ResourceManager.h"
#include "EngineLog.h"
#include "AudioManager.h"
#include "Texture.h"
#include "Engine.h"
#include "MajestroGameInstance.h"
#include "MathUtils.h"

namespace
{
	constexpr const wchar_t* kBackgroundKey  = L"UI_RhythmMan_Background";
	constexpr const wchar_t* kButtonSheetKey = L"UI_RhythmMan_Text_Shee";

	constexpr const wchar_t* kSheetKeys[PlayerType::Count] = {
		L"UI_RhythmMan_Rudwig_Sheet",  L"UI_RhythmMan_Ibanix_Sheet",  L"UI_RhythmMan_Fanthor_Sheet" };
	constexpr const wchar_t* kLineKeys[PlayerType::Count] = {
		L"UI_RhythmMan_Rudwig_Line",   L"UI_RhythmMan_Ibanix_Line",   L"UI_RhythmMan_Fanthor_Line" };
	constexpr const wchar_t* kPortraitKeys[PlayerType::Count] = {
		L"UI_RhythmMan_Rudwig_Portrait", L"UI_RhythmMan_Ibanix_Portrait", L"UI_RhythmMan_Fanthor_Portrait" };


	struct PreviewStem
	{
		SOUNDNAME stem;
		const char* eventPath;
		const char* parentParam;
		const char* subParam;
	};

	constexpr PreviewStem kPreviewStems[PlayerType::Count] = {
		{ SOUNDNAME::Drum, "event:/OST/DrumMulti", "DrumParam", "DrumSubParam" },
		{ SOUNDNAME::Bass, "event:/OST/BassMulti", "BassParam", "BassSubParam" },
		{ SOUNDNAME::Elec, "event:/OST/ElecMulti", "ElecParam", "ElecSubParam" },
	};

	// UI_RhythmMan_{캐릭터}_Sheet
	constexpr float kDiscCell = 512.f;

	// UI_RhythmMan_Text_Sheet
	constexpr RECT kBackTextRect   = { 10,  30,  122, 105 };
	constexpr RECT kPlayTextRect   = { 126, 30,  248, 105 };
	constexpr RECT kSaveTextRect   = { 252, 30,  393, 105 };
	constexpr RECT kArrowLeftRect  = { 10,  143, 94,  237 };
	constexpr RECT kArrowRightRect = { 160, 143, 246, 237 };



	const Vec2 kMainDiscPos   = { 430.f, -120.f };
	const Vec2 kMainDiscSize  = { 560.f, 560.f };
	const Vec2 kSubDiscPos[2] = { { 240.f, 330.f }, { 620.f, 330.f } };
	const Vec2 kSubDiscSize   = { 230.f, 230.f };
	const Vec2 kArrowLeftPos  = { 30.f,  -120.f };
	const Vec2 kArrowRightPos = { 830.f, -120.f };
	const Vec2 kArrowSize     = { 90.f,  100.f };
	const Vec2 kBackBtnPos    = { 110.f, 590.f };
	const Vec2 kPlayBtnPos    = { 430.f, 590.f };
	const Vec2 kSaveBtnPos    = { 700.f, 590.f };
	const Vec2 kTextBtnSize   = { 180.f, 110.f };
	const Vec2 kSaveBtnSize   = { 208.f, 110.f };  // Save 는 글자 폭이 넓어 Play 와 같은 픽셀 배율로 확장

	
	constexpr float kSpinOutDuration = 0.16f;  // 스핀아웃 (가속 회전 + 축소 + 페이드)
	constexpr float kSpinInDuration  = 0.26f;  // 스핀인 (감속 복귀)
	constexpr float kSpinOutAngle    = -2.6f;  // 라디안
	constexpr float kSpinInAngle     = 2.2f;
	constexpr float kSpinMinScale    = 0.72f;

	constexpr float kPulseDuration   = 0.55f;  // 링 한 바퀴 파면 시간
	constexpr float kPulseWidth      = 6.f;    // 파면 폭 (막대 수)

	constexpr float kBgmFadeSpeed      = 3.0f; // Ambient 음량 페이드 속도 (1/s)
	constexpr float kSaveFlashDuration = 0.35f;

	// 열림 연출
	struct IntroPhase { float delay; float duration; };
	constexpr IntroPhase kIntroBackground { 0.00f, 0.18f };  // 페이드
	constexpr IntroPhase kIntroLine       { 0.06f, 0.22f };  // 좌측 슬라이드인
	constexpr IntroPhase kIntroPortrait   { 0.12f, 0.26f };  // 좌측 슬라이드인
	constexpr IntroPhase kIntroDiscMain   { 0.15f, 0.32f };  // 스핀인 (+링 등장/펄스)
	constexpr IntroPhase kIntroDiscSub0   { 0.26f, 0.22f };  // 팝인
	constexpr IntroPhase kIntroDiscSub1   { 0.32f, 0.22f };  // 팝인
	constexpr IntroPhase kIntroButtons    { 0.36f, 0.22f };  // 팝인
	constexpr float kIntroTotalDuration   = 0.60f;

	constexpr float kIntroLineSlideRatio     = -0.05f;  // 화면 폭 비율
	constexpr float kIntroPortraitSlideRatio = -0.09f;
	constexpr float kIntroDiscSpinAngle      = 1.8f;    // 라디안
	constexpr float kIntroDiscMinScale       = 0.62f;
	constexpr float kIntroPopMinScale        = 0.85f;

	const Vec4 kDiscSubTint  = { 0.82f, 0.82f, 0.82f, 1.f };
	const Vec4 kPlayingTint  = { 0.5f, 1.f, 0.6f, 1.f };

	Entity CreateFullscreenSprite(World* world, uint8 layer, const wchar_t* texKey)
	{
		Entity e = world->CreateEntity();
		auto& tr = world->AddComponent<UITransformComponent>(e);
		tr.mAnchor = Anchor::Center;
		tr.mPivot = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = layer;
		tr.UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));

		auto& sp = world->AddComponent<UISpriteComponent>(e, RESOURCEMANAGER.Get<Texture>(texKey));
		sp.mVisible = false;
		return e;
	}
}

void UIRhythmSelectFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);

	AUDIOMANAGER.InitSpectrumDSP(2048);

	// 배경
	mBackground = CreateFullscreenSprite(world, 4, kBackgroundKey);
	mLine       = CreateFullscreenSprite(world, 5, kLineKeys[0]);
	mPortrait   = CreateFullscreenSprite(world, 6, kPortraitKeys[0]);

	// 메인 디스크
	{
		mDiscMain = world->CreateEntity();
		auto& tr = world->AddComponent<UITransformComponent>(mDiscMain);
		tr.mAnchor = Anchor::Center;
		tr.mPosition = kMainDiscPos;
		tr.mSize = kMainDiscSize;
		tr.mPivot = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = 7;

		auto& sp = world->AddComponent<UISpriteComponent>(mDiscMain,
			RESOURCEMANAGER.Get<Texture>(kSheetKeys[0]));
		sp.SetSourceRect(0.f, 0.f, kDiscCell, kDiscCell);
		sp.mVisible = false;
	}

	// 서브 디스크 2장
	for (int32 i = 0; i < 2; ++i)
	{
		UIButtonDesc desc;
		desc.anchor = Anchor::Center;
		desc.position = kSubDiscPos[i];
		desc.size = kSubDiscSize;
		desc.layer = 7;
		desc.visual = UIButtonVisual::Texture;
		desc.resKey = kSheetKeys[0];
		desc.sourceRect = { 0, 0, static_cast<LONG>(kDiscCell), static_cast<LONG>(kDiscCell) };
		desc.normalColor = kDiscSubTint;
		desc.hoveredColor = Colors::White.v;
		desc.hoveredScale = 1.06f;
		desc.onClick = [this, i]() { OnSubDiscClicked(i); };
		mDiscSubs[i] = CreateUIButton(world, desc);
	}

	// < >
	{
		UIButtonDesc desc;
		desc.anchor = Anchor::Center;
		desc.size = kArrowSize;
		desc.layer = 8;
		desc.visual = UIButtonVisual::Texture;
		desc.resKey = kButtonSheetKey;
		desc.hoveredScale = 1.15f;

		desc.position = kArrowLeftPos;
		desc.sourceRect = kArrowLeftRect;
		desc.onClick = [this]() { OnArrowClicked(-1); };
		mArrowLeft = CreateUIButton(world, desc);

		desc.position = kArrowRightPos;
		desc.sourceRect = kArrowRightRect;
		desc.onClick = [this]() { OnArrowClicked(+1); };
		mArrowRight = CreateUIButton(world, desc);
	}

	// Back / Play / Save
	{
		UIButtonDesc desc;
		desc.anchor = Anchor::Center;
		desc.size = kTextBtnSize;
		desc.layer = 8;
		desc.visual = UIButtonVisual::Texture;
		desc.resKey = kButtonSheetKey;
		desc.hoveredScale = 1.1f;

		desc.position = kBackBtnPos;
		desc.sourceRect = kBackTextRect;
		desc.onClick = [this]()
		{
			// 닫기만 요청
			if (DialogueStateComponent* state = mWorld->GetSingleton<DialogueStateComponent>())
				state->mRhythmSelectActive = false;
		};
		mBackButton = CreateUIButton(world, desc);

		desc.position = kPlayBtnPos;
		desc.sourceRect = kPlayTextRect;
		desc.onClick = [this]() { OnPlayClicked(); };
		mPlayButton = CreateUIButton(world, desc);

		desc.position = kSaveBtnPos;
		desc.size = kSaveBtnSize;
		desc.sourceRect = kSaveTextRect;
		desc.onClick = [this]() { OnSaveClicked(); };
		mSaveButton = CreateUIButton(world, desc);
	}

	// 메인 디스크를 감싸는 원형 비주얼라이저 링
	{
		mVisualizer = world->CreateEntity();
		auto& vis = world->AddComponent<CircularVisualizerComponent>(mVisualizer);
		vis.isVisible = false;
		vis.gain = 12.f;
		vis.riseSmooth = 40.f;
		vis.fallSmooth = 8.f;
	}

	// 대화창과 같은 Dialogue 그룹으로 렌더링
	const Entity groupEntities[] = {
		mBackground, mLine, mPortrait, mDiscMain, mDiscSubs[0], mDiscSubs[1],
		mArrowLeft, mArrowRight, mBackButton, mPlayButton, mSaveButton };
	for (Entity e : groupEntities)
		world->AddComponent<UIRenderGroupComponent>(e, UIRenderGroup::Dialogue);

	SetVisible(false);
}

void UIRhythmSelectFeature::Update(float dt)
{
	DialogueStateComponent* state = mWorld->GetSingleton<DialogueStateComponent>();
	if (!state)
		return;

	const bool active = state->mRhythmSelectActive;
	if (active != mVisible)
	{
		mVisible = active;
		if (active)
			Open();
		else
			Close();
		SetVisible(active);
		if (active)
			StartIntro();
	}

	UpdateBgmFade(dt);

	if (!mVisible)
		return;

	UpdateIntro(dt);
	UpdateDiscSpin(dt);
	UpdateVisualizer(dt);
	UpdateSaveFlash(dt);
}

void UIRhythmSelectFeature::Open()
{
	// 로컬 캐릭터 확인
	mPlayerType = static_cast<uint8>(PlayerType::Rudwig);
	if (mWorld->HasComponentPool<MainPlayerComponent>() &&
		mWorld->HasComponentPool<LocalPlayerComponent>())
	{
		auto locals = mWorld->GetEntitiesWithComponents<MainPlayerComponent, LocalPlayerComponent>();
		if (!locals.empty())
			if (MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(locals[0]))
				mPlayerType = static_cast<uint8>(player->mPlayerType);
	}

	if (mPlayerType >= static_cast<uint8>(PlayerType::Count))
		mPlayerType = static_cast<uint8>(PlayerType::Rudwig);

	// 로컬 선택 상태를 가져와서 UI에 반영
	const RhythmVariantSelection& selection =
		MajestroGameInstance::GetInstance().GetLocalRhythmVariantSelection();
	mColumns[0] = static_cast<int32>(selection.r1);
	mColumns[1] = static_cast<int32>(selection.r2);
	mColumns[2] = static_cast<int32>(selection.r3);
	mFocusRow = 0;
	mSubRows[0] = 1;
	mSubRows[1] = 2;

	mSpinActive = false;
	mPulseTime = -1.f;
	mSaveFlashTime = -1.f;

	ApplyCharacterAssets();
	ApplyDiscRects();
	ResetDiscVisuals();
	RefreshPlayButtonTint();
}

void UIRhythmSelectFeature::Close()
{
	StopPreview();        
	mSpinActive = false;
	mPulseTime = -1.f;
	mSaveFlashTime = -1.f;
	mIntroTime = -1.f;    
}

void UIRhythmSelectFeature::SetVisible(bool visible)
{
	auto setSprite = [this, visible](Entity e)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mVisible = visible;
	};
	auto setButton = [this, visible](Entity e)
	{
		if (UIButtonComponent* btn = mWorld->GetComponent<UIButtonComponent>(e))
			btn->mEnabled = visible;  // 숨김 중 히트테스트/클릭 방지
	};

	const Entity sprites[] = {
		mBackground, mLine, mPortrait, mDiscMain, mDiscSubs[0], mDiscSubs[1],
		mArrowLeft, mArrowRight, mBackButton, mPlayButton, mSaveButton };
	for (Entity e : sprites)
		setSprite(e);

	const Entity buttons[] = {
		mDiscSubs[0], mDiscSubs[1], mArrowLeft, mArrowRight,
		mBackButton, mPlayButton, mSaveButton };
	for (Entity e : buttons)
		setButton(e);

	if (CircularVisualizerComponent* vis =
			mWorld->GetComponent<CircularVisualizerComponent>(mVisualizer))
		vis->isVisible = visible;

	if (!visible)
		ResetDiscVisuals();
}

void UIRhythmSelectFeature::ApplyCharacterAssets()
{
	auto setTexture = [this](Entity e, const wchar_t* key)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mTexture = RESOURCEMANAGER.Get<Texture>(key);
	};

	setTexture(mLine, kLineKeys[mPlayerType]);
	setTexture(mPortrait, kPortraitKeys[mPlayerType]);
	setTexture(mDiscMain, kSheetKeys[mPlayerType]);
	setTexture(mDiscSubs[0], kSheetKeys[mPlayerType]);
	setTexture(mDiscSubs[1], kSheetKeys[mPlayerType]);
}

void UIRhythmSelectFeature::ApplyDiscRects()
{
	auto setCell = [this](Entity e, int32 row, int32 col)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->SetSourceRect(static_cast<float>(col) * kDiscCell,
				static_cast<float>(row) * kDiscCell, kDiscCell, kDiscCell);
	};

	setCell(mDiscMain, mFocusRow, mColumns[mFocusRow]);
	for (int32 i = 0; i < 2; ++i)
	{
		const int32 row = GetSubRow(i);
		setCell(mDiscSubs[i], row, mColumns[row]);
	}
}

void UIRhythmSelectFeature::ResetDiscVisuals()
{
	const Entity discs[] = { mDiscMain, mDiscSubs[0], mDiscSubs[1] };
	for (Entity e : discs)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
		{
			sp->mRotation = 0.f;
			sp->mColorTint.w = 1.f;
		}
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(e))
			tr->mScale = Vec2(1.f, 1.f);
	}
}

int32 UIRhythmSelectFeature::GetSubRow(int32 subIndex) const
{
	return mSubRows[std::clamp(subIndex, 0, 1)];
}

// 열림 연출
void UIRhythmSelectFeature::StartIntro()
{
	mIntroTime = 0.f;
	mIntroPulseFired = false;
	EnableButtons(false);


	if (CircularVisualizerComponent* vis =
			mWorld->GetComponent<CircularVisualizerComponent>(mVisualizer))
		vis->isVisible = false;
}

void UIRhythmSelectFeature::UpdateIntro(float dt)
{
	if (mIntroTime < 0.f)
		return;

	mIntroTime += dt;

	auto phase = [this](const IntroPhase& p)
	{
		return std::clamp((mIntroTime - p.delay) / p.duration, 0.f, 1.f);
	};
	auto setAlpha = [this](Entity e, float alpha)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mColorTint.w = alpha;
	};


	auto slideIn = [this, &setAlpha](Entity e, float t, float slideRatio)
	{
		const float eased = MathUtils::EaseOutCubic(t);
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(e))
			tr->mPositionRatio.x = slideRatio * (1.f - eased);
		setAlpha(e, eased);
	};

	auto popIn = [this, &setAlpha](Entity e, float t)
	{
		const float eased = MathUtils::EaseOutCubic(t);
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(e))
		{
			const float scale = kIntroPopMinScale + (1.f - kIntroPopMinScale) * eased;
			tr->mScale = Vec2(scale, scale);
		}
		setAlpha(e, eased);
	};

	setAlpha(mBackground, MathUtils::EaseOutCubic(phase(kIntroBackground)));
	slideIn(mLine, phase(kIntroLine), kIntroLineSlideRatio);
	slideIn(mPortrait, phase(kIntroPortrait), kIntroPortraitSlideRatio);

	// 메인 디스크
	{
		const float eased = MathUtils::EaseOutCubic(phase(kIntroDiscMain));
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(mDiscMain))
		{
			sp->mRotation = kIntroDiscSpinAngle * (1.f - eased);
			sp->mColorTint.w = eased;
		}
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(mDiscMain))
		{
			const float scale = kIntroDiscMinScale + (1.f - kIntroDiscMinScale) * eased;
			tr->mScale = Vec2(scale, scale);
		}
	}

	popIn(mDiscSubs[0], phase(kIntroDiscSub0));
	popIn(mDiscSubs[1], phase(kIntroDiscSub1));

	const float buttonT = phase(kIntroButtons);
	popIn(mArrowLeft, buttonT);
	popIn(mArrowRight, buttonT);
	popIn(mBackButton, buttonT);
	popIn(mPlayButton, buttonT);
	popIn(mSaveButton, buttonT);

	// 디스크 등장 시점
	if (mIntroTime >= kIntroDiscMain.delay)
	{
		if (CircularVisualizerComponent* vis =
				mWorld->GetComponent<CircularVisualizerComponent>(mVisualizer))
			vis->isVisible = true;

		if (!mIntroPulseFired)
		{
			mPulseTime = 0.f;
			mIntroPulseFired = true;
		}
	}

	if (mIntroTime >= kIntroTotalDuration)
		FinishIntro();
}

void UIRhythmSelectFeature::FinishIntro()
{
	mIntroTime = -1.f;

	// 모든 요소를 정지 상태로 확정
	ResetDiscVisuals();
	const Entity fadeTargets[] = {
		mBackground, mLine, mPortrait,
		mArrowLeft, mArrowRight, mBackButton, mPlayButton, mSaveButton };
	for (Entity e : fadeTargets)
	{
		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mColorTint.w = 1.f;
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(e))
			tr->mScale = Vec2(1.f, 1.f);
	}
	if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(mLine))
		tr->mPositionRatio.x = 0.f;
	if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(mPortrait))
		tr->mPositionRatio.x = 0.f;

	EnableButtons(true);
}

void UIRhythmSelectFeature::EnableButtons(bool enabled)
{
	const Entity buttons[] = {
		mDiscSubs[0], mDiscSubs[1], mArrowLeft, mArrowRight,
		mBackButton, mPlayButton, mSaveButton };
	for (Entity e : buttons)
	{
		if (UIButtonComponent* btn = mWorld->GetComponent<UIButtonComponent>(e))
			btn->mEnabled = enabled;
	}
}

// 전환 연출

void UIRhythmSelectFeature::StartDiscSpin(int32 subIndex)
{
	if (mSpinActive)
	{
		if (!mSpinRectApplied)
			ApplyDiscRects();  // 연타 시 직전 교체를 먼저 확정
		ResetDiscVisuals();    // 직전 스핀 대상이 다르면 잔여 회전/틴트 제거
	}

	mSpinTargets[0] = true;                 // 메인은 항상 교체 대상
	mSpinTargets[1] = (subIndex == 0);
	mSpinTargets[2] = (subIndex == 1);

	mSpinActive = true;
	mSpinRectApplied = false;
	mSpinTime = 0.f;
	mPulseTime = 0.f;      // 링 웨이브 펄스 동반
}

void UIRhythmSelectFeature::UpdateDiscSpin(float dt)
{
	if (!mSpinActive)
		return;

	mSpinTime += dt;

	float rotation = 0.f;
	float scale = 1.f;
	float alpha = 1.f;

	if (mSpinTime < kSpinOutDuration)
	{
		// 스핀아웃: 가속 회전하며 축소/페이드
		const float t = mSpinTime / kSpinOutDuration;
		const float tt = t * t;
		rotation = kSpinOutAngle * tt;
		scale = 1.f - (1.f - kSpinMinScale) * tt;
		alpha = 1.f - t;
	}
	else
	{
		if (!mSpinRectApplied)
		{
			ApplyDiscRects();  // 안 보이는 중간 지점에서 음반 교체
			mSpinRectApplied = true;
		}

		const float t = (mSpinTime - kSpinOutDuration) / kSpinInDuration;
		if (t >= 1.f)
		{
			mSpinActive = false;
		}
		else
		{
			// 스핀인: 반대 방향에서 감속하며 복귀
			const float eased = MathUtils::EaseOutCubic(t);
			rotation = kSpinInAngle * (1.f - eased);
			scale = kSpinMinScale + (1.f - kSpinMinScale) * eased;
			alpha = eased;
		}
	}

	const Entity discs[] = { mDiscMain, mDiscSubs[0], mDiscSubs[1] };
	for (int32 i = 0; i < 3; ++i)
	{
		if (!mSpinTargets[i])
			continue;

		if (UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(discs[i]))
		{
			sp->mRotation = rotation;
			sp->mColorTint.w = alpha;
		}
		if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(discs[i]))
			tr->mScale = Vec2(scale, scale);
	}
}

void UIRhythmSelectFeature::UpdateVisualizer(float dt)
{
	CircularVisualizerComponent* vis =
		mWorld->GetComponent<CircularVisualizerComponent>(mVisualizer);
	UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(mDiscMain);
	if (!vis || !tr)
		return;

	// 오디오 비쥬얼라이져 링 위치
	const float radius = tr->mFinalSize.x * 0.5f;
	vis->center = tr->mFinalPixelPos;
	vis->baseRadius = radius * 1.07f;
	vis->maxBarLength = radius * 0.34f;
	vis->minBarLength = std::max(2.f, radius * 0.02f);
	vis->barWidth = std::max(2.5f, radius * 0.016f);


	if (mPulseTime < 0.f)
		return;

	mPulseTime += dt;
	const float t = mPulseTime / kPulseDuration;
	if (t >= 1.f)
	{
		mPulseTime = -1.f;
		return;
	}

	const float front = t * static_cast<float>(CIRC_VIS_POINTS);
	const float fade = 1.f - t;
	for (int i = 0; i < CIRC_VIS_POINTS; ++i)
	{
		float dist = fabsf(static_cast<float>(i) - front);
		dist = std::min(dist, static_cast<float>(CIRC_VIS_POINTS) - dist);

		const float window = std::clamp(1.f - dist / kPulseWidth, 0.f, 1.f);
		const float amplitude = window * window * fade;
		vis->waveAmplitudes[i] = std::max(vis->waveAmplitudes[i], amplitude);
	}
}

void UIRhythmSelectFeature::UpdateSaveFlash(float dt)
{
	if (mSaveFlashTime < 0.f)
		return;

	mSaveFlashTime += dt;
	const float t = mSaveFlashTime / kSaveFlashDuration;

	float scale = 1.f;
	if (t >= 1.f)
		mSaveFlashTime = -1.f;
	else
		scale = 1.f + 0.25f * (1.f - MathUtils::EaseOutCubic(t));  // 팝 후 복귀

	if (UITransformComponent* tr = mWorld->GetComponent<UITransformComponent>(mSaveButton))
		tr->mScale = Vec2(scale, scale);
}

void UIRhythmSelectFeature::UpdateBgmFade(float dt)
{
	if (mAmbientVolume == mAmbientVolumeTarget)
		return;

	const float step = kBgmFadeSpeed * dt;
	mAmbientVolume += std::clamp(mAmbientVolumeTarget - mAmbientVolume, -step, step);
	AUDIOMANAGER.SetBGMVolume(SOUNDNAME::Ambient, mAmbientVolume);
}


void UIRhythmSelectFeature::OnArrowClicked(int32 direction)
{

	mColumns[mFocusRow] = (mColumns[mFocusRow] + direction + kColCount) % kColCount;
	StartDiscSpin(-1);
	ApplyPreviewParams();  
}

void UIRhythmSelectFeature::OnSubDiscClicked(int32 subIndex)
{

	std::swap(mFocusRow, mSubRows[subIndex]);
	StartDiscSpin(subIndex);
	ApplyPreviewParams();  // 재생 중이면 부모 리듬 전환
}

void UIRhythmSelectFeature::OnPlayClicked()
{
	if (mPreviewPlaying)
		StopPreview();
	else
		StartPreview();
	RefreshPlayButtonTint();
}

void UIRhythmSelectFeature::OnSaveClicked()
{
	MajestroGameInstance::GetInstance().SetLocalRhythmVariantSelection(
		SanitizeRhythmVariantSelection(
			static_cast<uint8>(mColumns[0]),
			static_cast<uint8>(mColumns[1]),
			static_cast<uint8>(mColumns[2])));
	mSaveFlashTime = 0.f;
}

void UIRhythmSelectFeature::StartPreview()
{
	const PreviewStem& config = kPreviewStems[mPlayerType];


	const bool prepared = AUDIOMANAGER.RequestBGM(config.eventPath, config.stem);
	if (!prepared)
	{
		EngineLog::WriteTaggedOnce(EngineLog::Domain::AudioDiagnostic, "rhythm-select",
			std::string("preview-failed:") + config.eventPath,
			"preview 재생 실패 event=", config.eventPath);
		return;
	}

	mPreviewPlaying = true;
	mAmbientVolumeTarget = 0.f;
	ApplyPreviewParams();
}

void UIRhythmSelectFeature::StopPreview()
{
	if (!mPreviewPlaying)
		return;

	AUDIOMANAGER.StopBGM(kPreviewStems[mPlayerType].stem);
	mPreviewPlaying = false;
	mAmbientVolumeTarget = 1.f;
	RefreshPlayButtonTint();
}

void UIRhythmSelectFeature::ApplyPreviewParams()
{
	if (!mPreviewPlaying)
		return;

	const PreviewStem& config = kPreviewStems[mPlayerType];

	AUDIOMANAGER.SetBGMParam(config.subParam, config.stem,
		static_cast<float>(mColumns[mFocusRow]), true);

	AUDIOMANAGER.SetBGMParam(config.parentParam, config.stem,
		static_cast<float>(mFocusRow + 1), true);
}

void UIRhythmSelectFeature::RefreshPlayButtonTint()
{
	UIButtonComponent* btn = mWorld->GetComponent<UIButtonComponent>(mPlayButton);
	UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(mPlayButton);
	if (!btn)
		return;

	const Vec4 tint = mPreviewPlaying ? kPlayingTint : Vec4(Colors::White.v);
	btn->mNormalColor = tint;

	if (sp && btn->mPrevState == ButtonVisualState::Normal)
		sp->mColorTint = tint;
}
