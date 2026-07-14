#include "pch.h"
#include "UIResultBoardFeature.h"

#include "Engine.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "GameRuleComponent.h"
#include "Network.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "CameraComponent.h"
#include "MathUtils.h"

namespace
{
	struct ResultRowSpec
	{
		float mSrcX, mSrcY, mSrcW, mSrcH; // 아틀라스 픽셀 소스 렉트
		float mPosY;                       // 행 중심 Y
		Vec4 mTextColor;                   // 숫자 텍스트 색
	};

	// 화면 우측 끝 고정
	constexpr float kBarRightX = -20.0f;
	const Vec2 kBarSize = Vec2(480.0f, 120.0f);

	// 숫자
	constexpr float kValueTextRightX = -155.0f;
	constexpr float kValueTextOffsetY = 22.0f; 

	constexpr uint8 kBarLayer = 251;  // Result 도장(250) 위
	constexpr uint8 kTextLayer = 252;

	// 표시 순서: 최상단부터 RESULT  -> TIME.
	const ResultRowSpec kRowSpecs[] = {
		{ 512.0f, 768.0f, 512.0f, 128.0f,  300.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // RESULT
		{   0.0f, 896.0f, 512.0f, 128.0f,  485.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // KILLS
		{   0.0f, 768.0f, 512.0f, 128.0f,  670.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // ASSIST
		{ 512.0f, 640.0f, 512.0f, 128.0f,  855.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // MAX COMBO
		{   0.0f, 640.0f, 512.0f, 128.0f, 1040.0f, Vec4(1.0f, 1.0f, 1.0f, 1.0f) }, // TIME
	};

	static_assert(
		sizeof(kRowSpecs) / sizeof(kRowSpecs[0]) == static_cast<size_t>(ResultBoardRow::Count),
		"kRowSpecs must cover every ResultBoardRow");

	// 랭크 문자(S/A/B/C/F)
	struct RankSpec
	{
		float mSrcX, mSrcY;
	};

	const RankSpec kRankSpecs[] = {
		{   0.0f,   0.0f }, // S
		{ 256.0f,   0.0f }, // A
		{ 512.0f,   0.0f }, // B
		{ 768.0f,   0.0f }, // C
		{ 512.0f, 256.0f }, // F
	};

	static_assert(
		sizeof(kRankSpecs) / sizeof(kRankSpecs[0]) == static_cast<size_t>(ResultRank::Count),
		"kRankSpecs must cover every ResultRank");

	constexpr float kRankCellSize = 256.0f;             // 아틀라스 셀 크기
	constexpr float kRankCenterX = -105.0f;             // 바 우측 끝에 겹치는 도장 (TopRight 기준)
	const Vec2 kRankSize = Vec2(150.0f, 150.0f);        // 항목별 등급 표시 크기
	constexpr float kRankRotation = -0.18f;             // 도장 각도 (라디안)
	constexpr uint8 kRankLayer = 253;                   // 숫자 텍스트(252) 위

	// 항목별 랭크 임계값
	struct RankThresholds
	{
		int32 mS, mA, mB;
	};
	const RankThresholds kRankThresholds[] = {
		{ 3000, 2000, 1200 }, // RESULT
		{   30,   20,   10 }, // KILLS
		{   15,    8,    4 }, // ASSIST
		{   50,   30,   15 }, // MAX COMBO
		{  240,  360,  480 }, // TIME (초 — 240 이하 S, 360 이하 A, 480 이하 B)
	};
	static_assert(
		sizeof(kRankThresholds) / sizeof(kRankThresholds[0]) == static_cast<size_t>(ResultBoardRow::Count),
		"kRankThresholds must cover every ResultBoardRow");
}

void UIResultBoardFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
	mActive = false;
}

void UIResultBoardFeature::Update(float dt)
{
	if (!mWorld)
		return;

	// Result 도장 등장 프레임에 연출 시작
	mWorld->GetEventManager()->Consume<EvResultBoardShow>([this](const EvResultBoardShow& ev) {
		StartPresentation(ev.phase);
	});

	if (mActive) // Result 도장 연출 중
	{
		// Clear/Fail 페이즈를 벗어나면 결과판을 정리
		const GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>();
		const WavePhaseType phase = rule
			? static_cast<WavePhaseType>(rule->mGamePhase)
			: WavePhaseType::None;

		if (phase != WavePhaseType::Clear && phase != WavePhaseType::Fail)
		{
			HideAll();
			mActive = false;
			return;
		}

		mElapsed += dt;

		// 룰렛 숫자는 모든 스핀 중인 행이 같은 박자로 갱신되도록 공용 틱을 사용
		mSpinTickAccum += dt;
		bool spinTick = false;
		if (mSpinTickAccum >= kSpinTickInterval)
		{
			mSpinTickAccum = std::fmod(mSpinTickAccum, kSpinTickInterval);
			spinTick = true;
		}

		for (size_t index = 0; index < kRowCount; ++index)
			UpdateRow(index, dt, spinTick);
		return;
	}

	// Tab
	UpdateQuickView(dt);
}

void UIResultBoardFeature::EnsureEntities()
{
	for (size_t index = 0; index < kRowCount; ++index)
	{
		RowState& row = mRows[index];
		const ResultRowSpec& spec = kRowSpecs[index];

		if (row.mBarEntity == NULL_ENTITY)
		{
			row.mBarEntity = mWorld->CreateEntity();

			auto& transform = mWorld->AddComponent<UITransformComponent>(row.mBarEntity);
			transform.mAnchor = Anchor::TopRight;
			transform.mPivot = Vec2(1.0f, 0.5f);
			transform.mSize = kBarSize;
			transform.mPosition = Vec2(kBarRightX, spec.mPosY);
			transform.mUILayerIndex = kBarLayer;

			auto& sprite = mWorld->AddComponent<UISpriteComponent>(row.mBarEntity);
			sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
			sprite.SetSourceRect(spec.mSrcX, spec.mSrcY, spec.mSrcW, spec.mSrcH);
			sprite.mVisible = false;
			sprite.mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);

			mWorld->AddComponent<UIRenderGroupComponent>(row.mBarEntity, UIRenderGroup::Clear);
		}

		if (row.mTextEntity == NULL_ENTITY)
		{
			row.mTextEntity = mWorld->CreateEntity();

			auto& transform = mWorld->AddComponent<UITransformComponent>(row.mTextEntity);
			transform.mAnchor = Anchor::TopRight;
			transform.mPivot = Vec2(1.0f, 0.5f);
			transform.mSize = Vec2(0.0f, 0.0f);
			transform.mPosition = Vec2(kValueTextRightX, spec.mPosY + kValueTextOffsetY);
			transform.mUILayerIndex = kTextLayer;

			auto& text = mWorld->AddComponent<UITextComponent>(row.mTextEntity);
			text.mFontType = UIFontType::Esamanru;
			text.mVisible = false;
			text.mOutlineThickness = 2.0f;
			text.mColor = DirectX::XMVECTORF32{ { {
				spec.mTextColor.x, spec.mTextColor.y, spec.mTextColor.z, 0.0f } } };

			mWorld->AddComponent<UIRenderGroupComponent>(row.mTextEntity, UIRenderGroup::Clear);
		}

		// 항목별 랭크 도장
		if (row.mRankEntity == NULL_ENTITY)
		{
			row.mRankEntity = mWorld->CreateEntity();

			auto& transform = mWorld->AddComponent<UITransformComponent>(row.mRankEntity);
			transform.mAnchor = Anchor::TopRight;
			transform.mPivot = Vec2(0.5f, 0.5f);
			transform.mSize = kRankSize;
			transform.mPosition = Vec2(kRankCenterX, spec.mPosY);
			transform.mUILayerIndex = kRankLayer;

			auto& sprite = mWorld->AddComponent<UISpriteComponent>(row.mRankEntity);
			sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
			sprite.mVisible = false;
			sprite.mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
			sprite.mRotation = kRankRotation;

			mWorld->AddComponent<UIRenderGroupComponent>(row.mRankEntity, UIRenderGroup::Clear);
		}
	}
}

void UIResultBoardFeature::StartPresentation(uint8 phaseRaw)
{
	EnsureEntities();

	// tab 끄기
	mQuickActive = false;
	mQuickElapsed = 0.0f;

	// 로컬 플레이어의 최신 서버 점수 스냅샷을 캡처
	std::array<int32, kRowCount> values{};
	ReadLiveValues(values);

	for (size_t index = 0; index < kRowCount; ++index)
		mRows[index].mFinalValue = values[index];

	// Fail 페이즈는 RESULT 항목 랭크가 F로 고정
	mIsFail = static_cast<WavePhaseType>(phaseRaw) == WavePhaseType::Fail;

	// 렌더 그룹 갱신
	const UIRenderGroup renderGroup =
		mIsFail ? UIRenderGroup::GameOver : UIRenderGroup::Clear;

	for (size_t index = 0; index < kRowCount; ++index)
	{
		RowState& row = mRows[index];
		row.mSpinStarted = false;
		row.mLocked = false;
		row.mRankStamped = false;
		row.mPopElapsed = -1.0f;

		if (auto* group = mWorld->GetComponent<UIRenderGroupComponent>(row.mBarEntity))
			group->mGroup = renderGroup;
		if (auto* group = mWorld->GetComponent<UIRenderGroupComponent>(row.mTextEntity))
			group->mGroup = renderGroup;
		if (auto* group = mWorld->GetComponent<UIRenderGroupComponent>(row.mRankEntity))
			group->mGroup = renderGroup;

		if (auto* rankSprite = mWorld->GetComponent<UISpriteComponent>(row.mRankEntity))
		{
			rankSprite->mVisible = false;
			rankSprite->mColorTint.w = 0.0f;
		}

		if (auto* rankTransform = mWorld->GetComponent<UITransformComponent>(row.mRankEntity))
			rankTransform->mScale = Vec2(kRankStartScale, kRankStartScale);

		if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(row.mBarEntity))
		{
			sprite->mVisible = false;
			sprite->mColorTint.w = 0.0f;
		}

		if (auto* transform = mWorld->GetComponent<UITransformComponent>(row.mBarEntity))
			transform->mPosition = Vec2(kBarRightX + kSlideOffsetX, kRowSpecs[index].mPosY);

		if (auto* text = mWorld->GetComponent<UITextComponent>(row.mTextEntity))
		{
			text->mVisible = false;
			text->mColor.f[3] = 0.0f;

			text->mText = FormatRowValue(
				static_cast<ResultBoardRow>(index), MakeSpinValue(row.mFinalValue));
		}

		if (auto* textTransform = mWorld->GetComponent<UITransformComponent>(row.mTextEntity))
			textTransform->mScale = Vec2(1.0f, 1.0f);
	}

	mActive = true;
	mElapsed = 0.0f;
	mSpinTickAccum = 0.0f;
}

void UIResultBoardFeature::UpdateRow(size_t index, float dt, bool spinTick)
{
	RowState& row = mRows[index];
	if (row.mBarEntity == NULL_ENTITY || row.mTextEntity == NULL_ENTITY)
		return;

	UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(row.mBarEntity);
	UITransformComponent* barTransform = mWorld->GetComponent<UITransformComponent>(row.mBarEntity);
	UITextComponent* text = mWorld->GetComponent<UITextComponent>(row.mTextEntity);
	UITransformComponent* textTransform = mWorld->GetComponent<UITransformComponent>(row.mTextEntity);
	if (!sprite || !barTransform || !text || !textTransform)
		return;

	// 행별 로컬 시간 순차 등장.
	const float local = mElapsed - static_cast<float>(index) * kRowStagger;
	if (local < 0.0f)
		return;

	// 페이드인 + 좌측 슬라이드.
	const float fadeT = std::clamp(local / kFadeDuration, 0.0f, 1.0f);
	const float eased = EaseOutCubic(fadeT);
	barTransform->mPosition.x = kBarRightX + kSlideOffsetX * (1.0f - eased);
	sprite->mVisible = true;
	sprite->mColorTint.w = fadeT;
	text->mVisible = true;
	text->mColor.f[3] = fadeT;

	if (!row.mSpinStarted)
	{
		row.mSpinStarted = true;
		RequestSfx("ui/switch"); // 룰렛 시작음(띠리리리 시작)
	}

	if (local < kSpinDuration)
	{
		// 룰렛 스핀
		if (spinTick)
			text->mText = FormatRowValue(
				static_cast<ResultBoardRow>(index), MakeSpinValue(row.mFinalValue));
	}
	else if (!row.mLocked)
	{
		// 락(똭)
		row.mLocked = true;
		text->mText = FormatRowValue(static_cast<ResultBoardRow>(index), row.mFinalValue);	// 최종값 확정
		row.mPopElapsed = 0.0f;		// 스케일 팝
		RequestSfx("ui/select");	// SFX
	}

	// 락 팝: 큰 스케일에서 1.0으로 빠르게 수렴.
	if (row.mPopElapsed >= 0.0f)
	{
		row.mPopElapsed += dt;
		const float popT = std::clamp(row.mPopElapsed / kPopDuration, 0.0f, 1.0f);
		const float scale = kPopStartScale + (1.0f - kPopStartScale) * EaseOutCubic(popT);
		textTransform->mScale = Vec2(scale, scale);
		if (popT >= 1.0f)
		{
			textTransform->mScale = Vec2(1.0f, 1.0f);
			row.mPopElapsed = -1.0f;
		}
	}

	// 항목별 랭크 도장
	UpdateRowRank(index, local - kSpinDuration - kRankStartDelay);
}

void UIResultBoardFeature::UpdateRowRank(size_t index, float rankLocal)
{
	RowState& row = mRows[index];

	UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(row.mRankEntity);
	UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(row.mRankEntity);
	if (!sprite || !transform)
		return;

	if (rankLocal < 0.0f)
	{
		sprite->mVisible = false;
		return;
	}

	// 등장 프레임 — 항목별 등급 지정하고 도장 임팩트
	if (!row.mRankStamped)
	{
		row.mRankStamped = true;

		const ResultRank rank = ComputeRankFor(
			static_cast<ResultBoardRow>(index), row.mFinalValue, mIsFail);
		const RankSpec& spec = kRankSpecs[static_cast<size_t>(rank)];
		sprite->SetSourceRect(spec.mSrcX, spec.mSrcY, kRankCellSize, kRankCellSize);
		sprite->mVisible = true;
		sprite->mColorTint.w = 0.0f;
		transform->mScale = Vec2(kRankStartScale, kRankStartScale);

		// 카메라 셰이크는 최상단(RESULT) 등급에서만
		if (static_cast<ResultBoardRow>(index) == ResultBoardRow::Result)
			TriggerCameraShake();
	}

	const float duration = (std::max)(kRankStampDuration, 0.001f);
	const float progress = std::clamp(rankLocal / duration, 0.0f, 1.0f);
	const float eased = EaseOutCubic(progress);
	const float scale = kRankStartScale + (1.0f - kRankStartScale) * eased;

	transform->mScale = Vec2(scale, scale);
	sprite->mColorTint.w = std::clamp(progress * 5.0f, 0.0f, 1.0f);
}

void UIResultBoardFeature::HideAll()
{
	for (RowState& row : mRows)
	{
		if (auto* sprite = mWorld->GetComponent<UISpriteComponent>(row.mBarEntity))
			sprite->mVisible = false;
		if (auto* text = mWorld->GetComponent<UITextComponent>(row.mTextEntity))
			text->mVisible = false;
		if (auto* rankSprite = mWorld->GetComponent<UISpriteComponent>(row.mRankEntity))
			rankSprite->mVisible = false;
	}
}

ResultRank UIResultBoardFeature::ComputeRankFor(ResultBoardRow row, int32 value, bool isFail)
{
	// 게임 오버 시 최종 점수 항목은 점수와 무관하게 F.
	if (isFail && row == ResultBoardRow::Result)
		return ResultRank::F;

	const RankThresholds& thresholds = kRankThresholds[static_cast<size_t>(row)];

	if (row == ResultBoardRow::Time)
	{
		// 클리어 시간은 짧을수록 상위 등급.
		if (value <= thresholds.mS)
			return ResultRank::S;
		if (value <= thresholds.mA)
			return ResultRank::A;
		if (value <= thresholds.mB)
			return ResultRank::B;
		return ResultRank::C;
	}

	if (value >= thresholds.mS)
		return ResultRank::S;
	if (value >= thresholds.mA)
		return ResultRank::A;
	if (value >= thresholds.mB)
		return ResultRank::B;
	return ResultRank::C;
}

void UIResultBoardFeature::ReadLiveValues(
	std::array<int32, static_cast<size_t>(ResultBoardRow::Count)>& outValues) const
{
	const ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();
	const uint32 myId = Network::GetInstance().mClientId;

	ClientPlayerScore my{};
	float gameTime = 0.0f;
	if (scoreBoard)
	{
		gameTime = scoreBoard->mGameTime;

		bool found = false;
		for (uint8 index = 0; index < scoreBoard->mPlayerCount; ++index)
		{
			if (scoreBoard->mPlayers[index].mSessionId == myId)
			{
				my = scoreBoard->mPlayers[index];
				found = true;
				break;
			}
		}

		if (!found && scoreBoard->mPlayerCount > 0)
			my = scoreBoard->mPlayers[0];
	}

	outValues[static_cast<size_t>(ResultBoardRow::Result)] = my.mScore;
	outValues[static_cast<size_t>(ResultBoardRow::Kills)] = my.mTotalKills;
	outValues[static_cast<size_t>(ResultBoardRow::Assist)] = my.mAssists;
	outValues[static_cast<size_t>(ResultBoardRow::MaxCombo)] = my.mMaxCombo;
	outValues[static_cast<size_t>(ResultBoardRow::Time)] = static_cast<int32>(gameTime);
}

void UIResultBoardFeature::SetRenderGroupAll(UIRenderGroup group)
{
	auto apply = [this, group](Entity entity)
	{
		if (entity == NULL_ENTITY)
			return;
		if (auto* component = mWorld->GetComponent<UIRenderGroupComponent>(entity))
			component->mGroup = group;
	};

	for (RowState& row : mRows)
	{
		apply(row.mBarEntity);
		apply(row.mTextEntity);
		apply(row.mRankEntity);
	}
}

void UIResultBoardFeature::UpdateQuickView(float dt)
{
	// 전투 페이즈에서만 열리고, Tab을 누르는 동안만 보임
	const GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>();
	const WavePhaseType phase = rule
		? static_cast<WavePhaseType>(rule->mGamePhase)
		: WavePhaseType::None;

	const bool phaseOk =
		phase == WavePhaseType::Conquest ||
		phase == WavePhaseType::Escort ||
		phase == WavePhaseType::Boss;

	const bool tabHeld =
		INPUT.GetKeyDown(eKeyCode::TAB) ||
		INPUT.GetKey(eKeyCode::TAB);

	if (!tabHeld || !phaseOk)
	{
		// Tab을 떼면 즉시 숨긴다.
		if (mQuickActive)
		{
			HideAll();
			mQuickActive = false;
		}
		return;
	}

	if (!mQuickActive)
	{
		mQuickActive = true;
		mQuickElapsed = 0.0f;
		EnsureEntities();
		SetRenderGroupAll(UIRenderGroup::Gameplay);
		RequestSfx("ui/hover");
	}

	mQuickElapsed += dt;

	// 값 반영
	std::array<int32, kRowCount> values{};
	ReadLiveValues(values);

	for (size_t index = 0; index < kRowCount; ++index)
	{
		RowState& row = mRows[index];
		UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(row.mBarEntity);
		UITransformComponent* barTransform = mWorld->GetComponent<UITransformComponent>(row.mBarEntity);
		UITextComponent* text = mWorld->GetComponent<UITextComponent>(row.mTextEntity);
		UITransformComponent* textTransform = mWorld->GetComponent<UITransformComponent>(row.mTextEntity);
		UISpriteComponent* rankSprite = mWorld->GetComponent<UISpriteComponent>(row.mRankEntity);
		UITransformComponent* rankTransform = mWorld->GetComponent<UITransformComponent>(row.mRankEntity);
		if (!sprite || !barTransform || !text || !textTransform || !rankSprite || !rankTransform)
			continue;

		const float local = mQuickElapsed - static_cast<float>(index) * kQuickRowStagger;
		if (local < 0.0f)
		{
			sprite->mVisible = false;
			text->mVisible = false;
			rankSprite->mVisible = false;
			continue;
		}

		// 빠른 페이드인 + 짧은 슬라이드
		const float fadeT = std::clamp(local / kQuickFadeDuration, 0.0f, 1.0f);
		const float eased = EaseOutCubic(fadeT);

		barTransform->mPosition = Vec2(
			kBarRightX + kQuickSlideOffsetX * (1.0f - eased), kRowSpecs[index].mPosY);
		sprite->mVisible = true;
		sprite->mColorTint.w = fadeT;

		text->mVisible = true;
		text->mColor.f[3] = fadeT;
		text->mText = FormatRowValue(static_cast<ResultBoardRow>(index), values[index]);
		textTransform->mScale = Vec2(1.0f, 1.0f);

		// 항목별 랭크 — 행과 함께 페이드인, 값 변화 시 등급 문자도 즉시 반영.
		const ResultRank rank = ComputeRankFor(
			static_cast<ResultBoardRow>(index), values[index], /*isFail=*/false);

		const RankSpec& rankSpec = kRankSpecs[static_cast<size_t>(rank)];
		rankSprite->SetSourceRect(rankSpec.mSrcX, rankSpec.mSrcY, kRankCellSize, kRankCellSize);
		rankSprite->mVisible = true;
		rankSprite->mColorTint.w = fadeT;

		rankTransform->mScale = Vec2(1.0f, 1.0f);
	}
}

void UIResultBoardFeature::TriggerCameraShake()
{
	if (!mWorld->HasComponentPool<CameraTypeComponent>())
		return;

	// 도장이 찍히는 프레임에 게임 카메라를 쉐이크
	for (Entity cameraEntity : mWorld->GetEntitiesWithComponent<CameraTypeComponent>())
	{
		if (CameraTypeComponent* camera = mWorld->GetComponent<CameraTypeComponent>(cameraEntity))
			camera->TriggerShake(Vec3(1.2f, 0.8f, 1.8f), 0.22f, 24.0f);
	}
}

int32 UIResultBoardFeature::MakeSpinValue(int32 finalValue)
{
	// 랜덤 값
	int32 digits = 1;
	for (int32 v = std::abs(finalValue); v >= 10; v /= 10)
		++digits;

	int32 low = 0;
	int32 high = 9;
	if (digits > 1)
	{
		low = 1;
		for (int32 i = 1; i < digits; ++i)
			low *= 10;          // 10^(digits-1)
		high = low * 10 - 1;    // 10^digits - 1
	}

	return RandomInt(low, high);
}

std::wstring UIResultBoardFeature::FormatRowValue(ResultBoardRow row, int32 value) const
{
	switch (row)
	{
	case ResultBoardRow::Time:
	{
		// mm'ss 표기
		const int32 totalSeconds = (std::max)(0, value);
		std::wstringstream stream;
		const int32 minutes = totalSeconds / 60;
		if (minutes < 10)
			stream << L"0";
		stream << minutes << L"'";
		const int32 seconds = totalSeconds % 60;
		if (seconds < 10)
			stream << L"0";
		stream << seconds;
		return stream.str();
	}
	case ResultBoardRow::Kills:
		return std::to_wstring(value) + L" KILL";
	case ResultBoardRow::Assist:
		return std::to_wstring(value) + L" ASSIST";
	case ResultBoardRow::MaxCombo:
		return std::to_wstring(value) + L" COMBO";
	case ResultBoardRow::Result:
	default:
		return std::to_wstring(value);
	}
}

void UIResultBoardFeature::RequestSfx(const char* key)
{
	if (key == nullptr || key[0] == '\0')
		return;

	if (auto eventManager = mWorld->GetEventManager())
		eventManager->Enqueue(EvSfxRequest{ key, Vec3::Zero });
}
