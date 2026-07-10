#include "pch.h"
#include "UIScoreBoardFeature.h"

#include "Engine.h"
#include "InputManager.h"
#include "GameRuleComponent.h"
#include "PlayerComponent.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "World.h"

#include <sstream>

namespace
{
	const wchar_t* GetScoreBoardPlayerName(uint8 playerType)
	{
		switch (static_cast<PlayerType>(playerType))
		{
		case PlayerType::Rudwig:
			return L"Rudwig";
		case PlayerType::Ibanix:
			return L"Ibanix";
		case PlayerType::Fanthor:
			return L"Fanthor";
		default:
			return L"Unknown";
		}
	}
}

void UIScoreBoardFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
	EnsureTextEntity();
	SetVisible(false);
}

void UIScoreBoardFeature::Update(float /*dt*/)
{
	if (!mWorld)
		return;

	const bool tabHeld =
		INPUT.GetKeyDown(eKeyCode::TAB) ||
		INPUT.GetKey(eKeyCode::TAB);
	const bool visible = tabHeld && CanOpenScoreBoard();

	if (!visible)
	{
		SetVisible(false);
		return;
	}

	EnsureTextEntity();
	UpdateText();
	SetVisible(true);
}

void UIScoreBoardFeature::EnsureTextEntity()
{
	if (!mWorld || mTextEntity != NULL_ENTITY)
		return;

	mTextEntity = mWorld->CreateEntity();

	auto& transform = mWorld->AddComponent<UITransformComponent>(mTextEntity);
	transform.mAnchor = Anchor::Center;
	transform.mPivot = Vec2(0.5f, 0.5f);
	transform.mSize = Vec2(1000.0f, 520.0f);
	transform.mPosition = Vec2(0.0f, 0.0f);
	transform.mUILayerIndex = 20;

	auto& text = mWorld->AddComponent<UITextComponent>(mTextEntity);
	text.mFontType = UIFontType::Esamanru;
	text.mVisible = false;
	text.mOutlineThickness = 2.0f;
}

void UIScoreBoardFeature::UpdateText()
{
	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity);
	if (!text)
		return;

	const ScoreBoardComponent* scoreBoard = mWorld->GetSingleton<ScoreBoardComponent>();

	std::wstringstream stream;
	stream << L"SCORE BOARD\n";

	if (!scoreBoard || scoreBoard->mPlayerCount == 0)
	{
		stream << L"RANK  PLAYER  KILLS  ASSIST  COMBO  RESULT\n";
		stream << L"NO SCORE DATA";
		text->mText = stream.str();
		return;
	}

	// 씬 진행 시간 mm:ss 표기.
	const int32 totalSeconds = static_cast<int32>(scoreBoard->mGameTime);
	stream << L"TIME " << (totalSeconds / 60) << L":";
	const int32 seconds = totalSeconds % 60;
	if (seconds < 10)
		stream << L"0";
	stream << seconds << L"\n";

	stream << L"RANK  PLAYER  KILLS  ASSIST  COMBO  RESULT\n";

	for (uint8 index = 0;
		index < scoreBoard->mPlayerCount && index < ROOM_MAX_PLAYERS;
		++index)
	{
		const ClientPlayerScore& player = scoreBoard->mPlayers[index];
		stream << static_cast<int32>(index + 1) << L".  "
			<< GetScoreBoardPlayerName(player.mPlayerType) << L"  "
			<< player.mTotalKills << L"  "
			<< player.mAssists << L"  "
			<< player.mMaxCombo << L"  "
			<< player.mScore << L"\n";
	}

	text->mText = stream.str();
}

void UIScoreBoardFeature::SetVisible(bool visible)
{
	if (!mWorld || mTextEntity == NULL_ENTITY)
		return;

	if (UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity))
		text->mVisible = visible;
}

bool UIScoreBoardFeature::CanOpenScoreBoard() const
{
	if (!mWorld)
		return false;

	const GameRuleComponent* rule = mWorld->GetSingleton<GameRuleComponent>();
	if (!rule)
		return false;

	// Prepare and result phases keep their existing UI instead of the live board.
	const WavePhaseType phase = static_cast<WavePhaseType>(rule->mGamePhase);
	return phase == WavePhaseType::Conquest ||
		phase == WavePhaseType::Escort ||
		phase == WavePhaseType::Boss;
}
