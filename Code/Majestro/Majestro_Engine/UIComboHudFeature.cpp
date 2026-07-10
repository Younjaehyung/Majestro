#include "pch.h"
#include "UIComboHudFeature.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "ComboComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"


namespace
{
	// 랭크 문자 셀 (각 256x256): S, A, B, C, F 순서 — UIResultBoardFeature와 동일.
	struct AtlasCell
	{
		float mSrcX, mSrcY;
	};

	const AtlasCell kHudRankCells[] = {
		{   0.0f,   0.0f }, // S
		{ 256.0f,   0.0f }, // A
		{ 512.0f,   0.0f }, // B
		{ 768.0f,   0.0f }, // C
		{ 512.0f, 256.0f }, // F
	};
	constexpr float kHudRankCellSize = 256.0f;

	// 판정 텍스트 셀 인덱스 = BeatJudgement (0=Miss, 1=Good, 2=Perfect).
	constexpr float kJudgeInset = 2.0f;
	const AtlasCell kJudgeCells[] = {
		{ 768.0f + kJudgeInset, 512.0f + kJudgeInset }, // MISS
		{ 512.0f + kJudgeInset, 512.0f + kJudgeInset }, // GOOD
		{   0.0f + kJudgeInset, 512.0f + kJudgeInset }, // PERFECT
	};
	constexpr float kJudgeCellW = 256.0f - kJudgeInset * 2.0f;
	constexpr float kJudgeCellH = 128.0f - kJudgeInset * 2.0f;

	// 랭크 뒤 스플래시 배경(BACK) 셀 — S/A 문자 아래 영역 (512x256).
	const AtlasCell kHudBackCell = { 0.0f, 256.0f };
	constexpr float kHudBackCellW = 512.0f;
	constexpr float kHudBackCellH = 256.0f;

	// "COMBO!" 브러시 텍스트 셀
	const AtlasCell kComboWordCell = { 256.0f, 512.0f };
	constexpr float kComboWordCellW = 256.0f;
	constexpr float kComboWordCellH = 128.0f;

	// 우상단 콤보 표시
	constexpr float kHudRankX = -150.0f;  // 랭크 문자 중심
	constexpr float kHudRankY = 140.0f;
	const Vec2 kHudRankSize = Vec2(225.0f, 225.0f); // 문자 실표시 높이

	// 랭크 뒤 스플래시 배경 — 화면 우상단 모서리에 붙는다 (pivot (1,0)).
	const Vec2 kHudBackSize = Vec2(450.0f, 315.0f);

	constexpr float kCountX = -197.0f; // 콤보 수 텍스트 중심
	constexpr float kCountY = 356.0f;  // 랭크 아래 (예시 y 267/1080)
	constexpr float kCountScale = 2.4f; // Rivera 기준 목표 높이 ≈ 95px

	constexpr float kComboWordX = -197.0f; // "COMBO!" 텍스처 중심
	constexpr float kComboWordY = 443.0f;  // 숫자 바로 아래 (예시 y 332/1080)
	const Vec2 kComboWordSize = Vec2(280.0f, 140.0f); // 텍스트 실표시 ≈ 196x47

	// 예시의 콤보 숫자 색상
	constexpr float kComboR = 1.0f, kComboG = 1.0f, kComboB = 1.0f;

	constexpr float kHudPopDuration = 0.15f; // 증가/랭크 변경 팝 길이
	constexpr float kCountPopScale = 1.45f;
	constexpr float kHudRankPopScale = 1.35f;

	// 판정 피드
	constexpr float kFeedBaseY = -60.0f;   // 최신 판정(가장 아래) 중심 Y
	constexpr float kFeedSpacing = 72.0f;  // 위로 밀리는 간격
	const Vec2 kFeedSize = Vec2(232.0f, 116.0f);
	constexpr float kFeedSpawnScale = 1.4f; // 등장 시작 스케일
	constexpr float kFeedSpawnYOffset = 36.0f; // 아래에서 올라오며 등장
	constexpr float kFeedLerpSpeed = 12.0f; // 위치/스케일/알파 수렴 속도

	// 슬롯(0=최신)별 목표 스케일/알파
	constexpr float kSlotScale[] = { 1.0f, 0.8f, 0.65f };
	constexpr float kSlotAlpha[] = { 1.0f, 0.7f, 0.45f };

	constexpr uint8 kFeedLayer = 6;
	constexpr uint8 kComboBackLayer = 6; // 스플래시 배경 (랭크 아래)
	constexpr uint8 kComboLayer = 7;     // 랭크 문자 / COMBO! 텍스처
	constexpr uint8 kComboTextLayer = 8; // 콤보 숫자 (가장 위)

	float LerpTo(float current, float target, float dt)
	{
		return current + (target - current) * (std::min)(1.0f, dt * kFeedLerpSpeed);
	}
}


void UIComboHudFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
	mFeedEntries.clear();
	mFeedPoolUsed.fill(false);
	mLastCombo = 0;
	mCurrentRankIndex = kRankIndexF; // 기본 랭크 F에서 시작
	mAppliedRankIndex = -1;
	mRankDecayTimer = 0.0f;
}

void UIComboHudFeature::Update(float dt)
{
	if (!mWorld)
		return;

	EnsureEntities();

	mWorld->GetEventManager()->Consume<EvBeatJudgement>([this](const EvBeatJudgement& ev) {
		if (ev.predicted)
			PushJudgement(ev.judgement);
	});

	UpdateComboCorner(dt);
	UpdateJudgementFeed(dt);
}

void UIComboHudFeature::EnsureEntities()
{
	// 랭크 뒤 스플래시 배경 
	if (mRankBackEntity == NULL_ENTITY)
	{
		mRankBackEntity = mWorld->CreateEntity();

		auto& transform = mWorld->AddComponent<UITransformComponent>(mRankBackEntity);
		transform.mAnchor = Anchor::TopRight;
		transform.mPivot = Vec2(1.0f, 0.0f);
		transform.mSize = kHudBackSize;
		transform.mPosition = Vec2(0.0f, 0.0f);
		transform.mUILayerIndex = kComboBackLayer;

		auto& sprite = mWorld->AddComponent<UISpriteComponent>(mRankBackEntity);
		sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
		sprite.SetSourceRect(kHudBackCell.mSrcX, kHudBackCell.mSrcY, kHudBackCellW, kHudBackCellH);
		sprite.mVisible = false;

		mWorld->AddComponent<UIRenderGroupComponent>(mRankBackEntity, UIRenderGroup::Gameplay);
	}

	// 우상단 랭크 문자.
	if (mRankEntity == NULL_ENTITY)
	{
		mRankEntity = mWorld->CreateEntity();

		auto& transform = mWorld->AddComponent<UITransformComponent>(mRankEntity);
		transform.mAnchor = Anchor::TopRight;
		transform.mPivot = Vec2(0.5f, 0.5f);
		transform.mSize = kHudRankSize;
		transform.mPosition = Vec2(kHudRankX, kHudRankY);
		transform.mUILayerIndex = kComboLayer;

		auto& sprite = mWorld->AddComponent<UISpriteComponent>(mRankEntity);
		sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
		sprite.mVisible = false;

		mWorld->AddComponent<UIRenderGroupComponent>(mRankEntity, UIRenderGroup::Gameplay);
	}

	// 콤보 수 Font Text (Rivera)
	if (mCountTextEntity == NULL_ENTITY)
	{
		mCountTextEntity = mWorld->CreateEntity();

		auto& transform = mWorld->AddComponent<UITransformComponent>(mCountTextEntity);
		transform.mAnchor = Anchor::TopRight;
		transform.mPivot = Vec2(0.5f, 0.5f);
		transform.mSize = Vec2(0.0f, 0.0f);
		transform.mPosition = Vec2(kCountX, kCountY);
		transform.mScale = Vec2(kCountScale, kCountScale);
		transform.mUILayerIndex = kComboTextLayer;

		auto& text = mWorld->AddComponent<UITextComponent>(mCountTextEntity);
		text.mFontType = UIFontType::Rivera; // 콤보 전용 브러시 스타일 폰트
		text.mVisible = false;
		text.mOutlineThickness = 3.0f;
		text.mColor = DirectX::XMVECTORF32{ { { kComboR, kComboG, kComboB, 1.0f } } };

		mWorld->AddComponent<UIRenderGroupComponent>(mCountTextEntity, UIRenderGroup::Gameplay);
	}

	// "COMBO!"
	if (mComboWordEntity == NULL_ENTITY)
	{
		mComboWordEntity = mWorld->CreateEntity();

		auto& transform = mWorld->AddComponent<UITransformComponent>(mComboWordEntity);
		transform.mAnchor = Anchor::TopRight;
		transform.mPivot = Vec2(0.5f, 0.5f);
		transform.mSize = kComboWordSize;
		transform.mPosition = Vec2(kComboWordX, kComboWordY);
		transform.mUILayerIndex = kComboLayer;

		auto& sprite = mWorld->AddComponent<UISpriteComponent>(mComboWordEntity);
		sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
		sprite.SetSourceRect(kComboWordCell.mSrcX, kComboWordCell.mSrcY, kComboWordCellW, kComboWordCellH);
		sprite.mVisible = false;

		mWorld->AddComponent<UIRenderGroupComponent>(mComboWordEntity, UIRenderGroup::Gameplay);
	}

	// 판정 피드
	for (size_t index = 0; index < kFeedPoolSize; ++index)
	{
		if (mFeedPool[index] != NULL_ENTITY)
			continue;

		mFeedPool[index] = mWorld->CreateEntity();

		auto& transform = mWorld->AddComponent<UITransformComponent>(mFeedPool[index]);
		transform.mAnchor = Anchor::Center;
		transform.mPivot = Vec2(0.5f, 0.5f);
		transform.mSize = kFeedSize;
		transform.mPosition = Vec2(0.0f, kFeedBaseY);
		transform.mUILayerIndex = kFeedLayer;

		auto& sprite = mWorld->AddComponent<UISpriteComponent>(mFeedPool[index]);
		sprite.mTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Result_Combo_Sheet");
		sprite.mVisible = false;
		sprite.mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);

		mWorld->AddComponent<UIRenderGroupComponent>(mFeedPool[index], UIRenderGroup::Gameplay);
	}
}

int UIComboHudFeature::ReadLocalCombo() const
{
	if (!mWorld->HasComponentPool<LocalPlayerComponent>())
		return 0;

	const std::vector<Entity> players = mWorld->GetEntitiesWithComponent<LocalPlayerComponent>();
	if (players.empty())
		return 0;

	if (const ComboComponent* combo = mWorld->GetComponent<ComboComponent>(players.front()))
		return combo->mCount;
	return 0;
}

int UIComboHudFeature::RankIndexForCombo(int combo)
{
	// 콤보 구간별 도달 랭크 (kHudRankCells 인덱스)
	if (combo >= 30) return 0;      // S
	if (combo >= 20) return 1;      // A
	if (combo >= 10) return 2;      // B
	if (combo >= 1)  return 3;      // C
	return kRankIndexF;             // F (기본)
}

void UIComboHudFeature::UpdateRankState(int combo, float dt)
{
	if (combo > 0)
	{
		// 콤보 활동 중
		mRankDecayTimer = 0.0f;
		const int candidate = RankIndexForCombo(combo);
		if (candidate < mCurrentRankIndex)
		{
			mCurrentRankIndex = candidate;
			mRankPopTimer = kHudPopDuration;
		}
		return;
	}

	if (mCurrentRankIndex >= kRankIndexF)
	{
		// F — 기본 랭크로 유지
		mRankDecayTimer = 0.0f;
		return;
	}

	// 콤보가 끊긴 순간(타임아웃/Miss) 즉시 한 단계 하강
	if (mLastCombo > 0)
	{
		++mCurrentRankIndex;
		mRankPopTimer = kHudPopDuration;
		mRankDecayTimer = 0.0f;
		return;
	}

	// 이후에도 콤보가 없으면 일정 간격마다 한 단계씩 F까지 하강
	mRankDecayTimer += dt;
	if (mRankDecayTimer >= kRankDecayInterval)
	{
		mRankDecayTimer = 0.0f;
		++mCurrentRankIndex;
		mRankPopTimer = kHudPopDuration;
	}
}

void UIComboHudFeature::UpdateComboCorner(float dt)
{
	UISpriteComponent* backSprite = mWorld->GetComponent<UISpriteComponent>(mRankBackEntity);
	UISpriteComponent* rankSprite = mWorld->GetComponent<UISpriteComponent>(mRankEntity);
	UITransformComponent* rankTransform = mWorld->GetComponent<UITransformComponent>(mRankEntity);
	UITextComponent* countText = mWorld->GetComponent<UITextComponent>(mCountTextEntity);
	UITransformComponent* countTransform = mWorld->GetComponent<UITransformComponent>(mCountTextEntity);
	UISpriteComponent* wordSprite = mWorld->GetComponent<UISpriteComponent>(mComboWordEntity);

	if (!backSprite || !rankSprite || !rankTransform || !countText || !countTransform || !wordSprite)
		return;

	const int combo = ReadLocalCombo();

	// 랭크 상태 갱신
	UpdateRankState(combo, dt);

	// 랭크 반영  (기본 F)
	if (mCurrentRankIndex != mAppliedRankIndex)
	{
		const AtlasCell& cell = kHudRankCells[mCurrentRankIndex];
		rankSprite->SetSourceRect(cell.mSrcX, cell.mSrcY, kHudRankCellSize, kHudRankCellSize);
		mAppliedRankIndex = mCurrentRankIndex;
	}

	mRankPopTimer = (std::max)(0.0f, mRankPopTimer - dt);
	const float rankScale = std::lerp(1.0f, kHudRankPopScale, mRankPopTimer / kHudPopDuration);
	rankTransform->mScale = Vec2(rankScale, rankScale);

	backSprite->mVisible = true;
	rankSprite->mVisible = true;
	rankSprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (combo <= 0)
	{
		countText->mVisible = false;
		wordSprite->mVisible = false;
		mLastCombo = 0;
		return;
	}

	// 콤보 수 갱신 + 증가 시 팝.
	if (combo != mLastCombo)
	{
		countText->mText = std::to_wstring(combo);
		if (combo > mLastCombo)
			mCountPopTimer = kHudPopDuration;
		mLastCombo = combo;
	}

	mCountPopTimer = (std::max)(0.0f, mCountPopTimer - dt);

	const float countScale =
		kCountScale * std::lerp(1.0f, kCountPopScale, mCountPopTimer / kHudPopDuration);

	countTransform->mScale = Vec2(countScale, countScale);

	countText->mVisible = true;
	wordSprite->mVisible = true;
}

void UIComboHudFeature::PushJudgement(uint8 judgement)
{
	if (judgement >= sizeof(kJudgeCells) / sizeof(kJudgeCells[0]))
		return;

	// 빈 풀 슬롯 확보
	size_t poolIndex = kFeedPoolSize;
	for (size_t index = 0; index < kFeedPoolSize; ++index)
	{
		if (!mFeedPoolUsed[index])
		{
			poolIndex = index;
			break;
		}
	}

	if (poolIndex == kFeedPoolSize && !mFeedEntries.empty())
	{
		const JudgementEntry& oldest = mFeedEntries.back();
		poolIndex = oldest.mPoolIndex;
		if (UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(mFeedPool[poolIndex]))
			sprite->mVisible = false;
		mFeedEntries.pop_back();
	}

	if (poolIndex >= kFeedPoolSize)
		return;

	mFeedPoolUsed[poolIndex] = true;

	JudgementEntry entry;
	entry.mPoolIndex = poolIndex;
	entry.mJudgement = judgement;
	entry.mCurrentY = kFeedBaseY + kFeedSpawnYOffset; // 아래에서 올라오며 등장
	entry.mCurrentScale = kFeedSpawnScale;
	entry.mCurrentAlpha = 0.0f;
	mFeedEntries.push_front(entry);

	const AtlasCell& cell = kJudgeCells[judgement];

	if (UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(mFeedPool[poolIndex]))
	{
		sprite->SetSourceRect(cell.mSrcX, cell.mSrcY, kJudgeCellW, kJudgeCellH);
		sprite->mVisible = true;
		sprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, 0.0f);
	}
}

void UIComboHudFeature::UpdateJudgementFeed(float dt)
{
	// 최신부터 순서대로 살아있는 항목에 슬롯 인덱스를 부여한다.
	int liveIndex = 0;
	for (JudgementEntry& entry : mFeedEntries)
	{
		entry.mAge += dt;

		if (!entry.mFadingOut)
		{
			// 3개를 넘겨 밀려나거나 3초 타임아웃 시 페이드 아웃 시작.
			if (liveIndex >= kFeedMaxVisible || entry.mAge >= kFeedTimeout)
			{
				entry.mFadingOut = true;
				entry.mFadeElapsed = 0.0f;
				entry.mAlphaAtFadeStart = entry.mCurrentAlpha;
			}
		}

		UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(mFeedPool[entry.mPoolIndex]);
		UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(mFeedPool[entry.mPoolIndex]);
		if (!sprite || !transform)
			continue;

		if (entry.mFadingOut)
		{
			// 현재 위치/스케일을 유지한 채 알파만 자연 감쇠.
			entry.mFadeElapsed += dt;
			const float fadeT = std::clamp(entry.mFadeElapsed / kFeedFadeDuration, 0.0f, 1.0f);
			entry.mCurrentAlpha = entry.mAlphaAtFadeStart * (1.0f - fadeT);
			sprite->mColorTint.w = entry.mCurrentAlpha;
			continue;
		}

		// 슬롯 목표값으로 부드럽게 수렴 — 새 판정이 오면 이전 판정이 위로 밀려 올라간다.
		const int slot = (std::min)(liveIndex, kFeedMaxVisible - 1);
		const float targetY = kFeedBaseY - kFeedSpacing * static_cast<float>(liveIndex);

		entry.mCurrentY = LerpTo(entry.mCurrentY, targetY, dt);
		entry.mCurrentScale = LerpTo(entry.mCurrentScale, kSlotScale[slot], dt);
		entry.mCurrentAlpha = LerpTo(entry.mCurrentAlpha, kSlotAlpha[slot], dt);

		transform->mPosition = Vec2(0.0f, entry.mCurrentY);
		transform->mScale = Vec2(entry.mCurrentScale, entry.mCurrentScale);
		sprite->mColorTint.w = entry.mCurrentAlpha;

		++liveIndex;
	}

	// 페이드가 끝난 항목은 풀에 반납하고 목록에서 제거.
	for (auto it = mFeedEntries.begin(); it != mFeedEntries.end();)
	{
		if (it->mFadingOut && it->mFadeElapsed >= kFeedFadeDuration)
		{
			if (UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(mFeedPool[it->mPoolIndex]))
				sprite->mVisible = false;
			mFeedPoolUsed[it->mPoolIndex] = false;
			it = mFeedEntries.erase(it);
		}
		else
		{
			++it;
		}
	}
}
