#include "pch.h"
#include "PlayerStatusUIFeature.h"

#include "PlayerStatusComponent.h"
#include "TagComponent.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "World.h"

#include <sstream>

void PlayerStatusUIFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);

	PlayerStatusUISpec silence;
	silence.mTitle = L"MUSIC MUTED";
	silence.mPriority = 100;
	silence.mColor = Vec4(0.75f, 0.85f, 1.0f, 1.0f);
	mSpecs[PlayerStatusUIType::Silenced] = silence;

	PlayerStatusUISpec stun;
	stun.mTitle = L"STUNNED";
	stun.mPriority = 200;
	stun.mIntroStartScale = 1.5f;
	stun.mColor = Vec4(1.0f, 0.85f, 0.25f, 1.0f);
	mSpecs[PlayerStatusUIType::Stunned] = stun;

	PlayerStatusUISpec dead;
	dead.mTitle = L"RESPAWNING";
	dead.mPriority = 300;
	dead.mIntroDuration = 0.3f;
	dead.mIntroStartScale = 1.15f;
	dead.mColor = Vec4(1.0f, 0.3f, 0.3f, 1.0f);
	mSpecs[PlayerStatusUIType::Dead] = dead;

	EnsureUI();
	SetVisible(false);
}

void PlayerStatusUIFeature::Update(float dt)
{
	if (!mWorld)
		return;

	const Entity player = FindLocalPlayer();
	if (player == NULL_ENTITY)
	{
		ChangeStatus(PlayerStatusUIType::None);
		TickAnimation(dt);
		return;
	}

	if (PlayerStatusComponent* status = mWorld->GetComponent<PlayerStatusComponent>(player))
	{
		for (uint8 index = 0;
			index < status->mBuffCount && index < MAX_REPLICATED_BUFFS;
			++index)
		{
			ReplicatedBuffState& buff = status->mBuffs[index];
			if (buff.remainingTime >= 0.0f)
				buff.remainingTime = max(0.0f, buff.remainingTime - dt);
		}

		status->mStunRemaining = max(0.0f, status->mStunRemaining - dt);
		status->mRespawnRemaining = max(0.0f, status->mRespawnRemaining - dt);
	}

	ChangeStatus(SelectStatus(player));
	TickAnimation(dt);
	UpdateText(player);
}

Entity PlayerStatusUIFeature::FindLocalPlayer() const
{
	if (!mWorld->HasComponentPool<LocalPlayerComponent>())
		return NULL_ENTITY;

	const std::vector<Entity> players =
		mWorld->GetEntitiesWithComponent<LocalPlayerComponent>();
	return players.empty() ? NULL_ENTITY : players.front();
}

PlayerStatusUIType PlayerStatusUIFeature::SelectStatus(Entity player) const
{
	const PlayerStatusComponent* status =
		mWorld->GetComponent<PlayerStatusComponent>(player);
	if (!status)
		return PlayerStatusUIType::None;

	PlayerStatusUIType selected = PlayerStatusUIType::None;
	int32 selectedPriority = -1;

	auto consider = [&](PlayerStatusUIType type, uint8 flag)
	{
		if ((status->mStatusFlags & flag) == 0)
			return;

		const auto found = mSpecs.find(type);
		if (found != mSpecs.end() && found->second.mPriority > selectedPriority)
		{
			selected = type;
			selectedPriority = found->second.mPriority;
		}
	};

	if (status->FindBuff(ReplicatedBuffType::Silence))
	{
		const auto found = mSpecs.find(PlayerStatusUIType::Silenced);
		if (found != mSpecs.end() && found->second.mPriority > selectedPriority)
		{
			selected = PlayerStatusUIType::Silenced;
			selectedPriority = found->second.mPriority;
		}
	}

	consider(PlayerStatusUIType::Stunned, PlayerStatus_Stunned);
	consider(PlayerStatusUIType::Dead, PlayerStatus_Dead);
	return selected;
}

float PlayerStatusUIFeature::GetRemaining(Entity player, PlayerStatusUIType type) const
{
	const PlayerStatusComponent* status =
		mWorld->GetComponent<PlayerStatusComponent>(player);
	if (!status)
		return 0.0f;

	switch (type)
	{
	case PlayerStatusUIType::Silenced:
		return status->GetBuffRemaining(ReplicatedBuffType::Silence);
	case PlayerStatusUIType::Stunned:
		return status->mStunRemaining;
	case PlayerStatusUIType::Dead:
		return status->mRespawnRemaining;
	default:
		return 0.0f;
	}
}

void PlayerStatusUIFeature::ChangeStatus(PlayerStatusUIType nextStatus)
{
	if (mStage == PlayerStatusUIStage::Outro)
	{
		if (nextStatus == mPendingStatus)
			return;

		if (nextStatus == mCurrentStatus)
		{
			mPendingStatus = PlayerStatusUIType::None;
			mStage = PlayerStatusUIStage::Hold;
			mStageElapsed = 0.0f;
			return;
		}

		mPendingStatus = nextStatus;
		mStageElapsed = 0.0f;
		return;
	}

	if (nextStatus == mCurrentStatus)
		return;

	if (mCurrentStatus == PlayerStatusUIType::None)
	{
		mCurrentStatus = nextStatus;
		mStage = nextStatus == PlayerStatusUIType::None
			? PlayerStatusUIStage::Idle
			: PlayerStatusUIStage::Intro;
		mStageElapsed = 0.0f;
		SetVisible(nextStatus != PlayerStatusUIType::None);
		return;
	}

	mPendingStatus = nextStatus;
	mStage = PlayerStatusUIStage::Outro;
	mStageElapsed = 0.0f;
}

void PlayerStatusUIFeature::TickAnimation(float dt)
{
	if (mStage == PlayerStatusUIStage::Idle)
		return;

	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity);
	UITransformComponent* transform =
		mWorld->GetComponent<UITransformComponent>(mTextEntity);
	if (!text || !transform)
		return;

	mStageElapsed += max(0.0f, dt);
	const PlayerStatusUISpec& spec = mSpecs[mCurrentStatus];

	switch (mStage)
	{
	case PlayerStatusUIStage::Intro:
	{
		const float progress =
			std::clamp(mStageElapsed / max(spec.mIntroDuration, 0.001f), 0.0f, 1.0f);
		const float eased = 1.0f - std::pow(1.0f - progress, 3.0f);
		const float scale =
			1.0f + (spec.mIntroStartScale - 1.0f) * (1.0f - eased);
		transform->mScale = Vec2(scale, scale);
		text->mColor.f[3] = progress;

		if (progress >= 1.0f)
		{
			mStage = PlayerStatusUIStage::Hold;
			mStageElapsed = 0.0f;
		}
		break;
	}
	case PlayerStatusUIStage::Hold:
		transform->mScale = Vec2(1.0f, 1.0f);
		text->mColor.f[3] = 1.0f;
		break;
	case PlayerStatusUIStage::Outro:
	{
		const float progress =
			std::clamp(mStageElapsed / max(spec.mOutroDuration, 0.001f), 0.0f, 1.0f);
		text->mColor.f[3] = 1.0f - progress;
		transform->mScale = Vec2(1.0f - progress * 0.1f, 1.0f - progress * 0.1f);

		if (progress >= 1.0f)
		{
			mCurrentStatus = mPendingStatus;
			mPendingStatus = PlayerStatusUIType::None;
			mStageElapsed = 0.0f;

			if (mCurrentStatus == PlayerStatusUIType::None)
			{
				mStage = PlayerStatusUIStage::Idle;
				SetVisible(false);
			}
			else
			{
				mStage = PlayerStatusUIStage::Intro;
				SetVisible(true);
			}
		}
		break;
	}
	default:
		break;
	}
}

void PlayerStatusUIFeature::UpdateText(Entity player)
{
	if (mCurrentStatus == PlayerStatusUIType::None)
		return;

	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity);
	if (!text)
		return;

	const PlayerStatusUISpec& spec = mSpecs[mCurrentStatus];
	const float remaining = GetRemaining(player, mCurrentStatus);

	std::wstringstream stream;
	stream << spec.mTitle;
	if (remaining >= 0.0f)
		stream << L"\n" << static_cast<int32>(std::ceil(remaining));

	const float alpha = text->mColor.f[3];
	text->mText = stream.str();
	text->mColor = DirectX::XMVECTORF32{
		{ { spec.mColor.x, spec.mColor.y, spec.mColor.z, alpha } }
	};
}

void PlayerStatusUIFeature::EnsureUI()
{
	if (mTextEntity != NULL_ENTITY)
		return;

	mTextEntity = mWorld->CreateEntity();

	auto& transform = mWorld->AddComponent<UITransformComponent>(mTextEntity);
	transform.mAnchor = Anchor::Center;
	transform.mPivot = Vec2(0.5f, 0.5f);
	transform.mPosition = Vec2(0.0f, 0.0f);
	transform.mSize = Vec2(900.0f, 240.0f);
	transform.mUILayerIndex = 30;

	auto& text = mWorld->AddComponent<UITextComponent>(mTextEntity);
	text.mFontType = UIFontType::Esamanru;
	text.mVisible = false;
	text.mOutlineThickness = 3.0f;
	text.mColor = DirectX::Colors::White;
}

void PlayerStatusUIFeature::SetVisible(bool visible)
{
	if (UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity))
		text->mVisible = visible;
}
