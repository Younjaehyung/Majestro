#include "pch.h"
#include "PlayerStatusUIFeature.h"

#include "PlayerComponent.h"
#include "PlayerStatusComponent.h"
#include "ResourceManager.h"
#include "TagComponent.h"
#include "Texture.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
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
	UpdateVisual(player);
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

	const bool useMuteIcon = mCurrentStatus == PlayerStatusUIType::Silenced;
	UITransformComponent* transform =
		mWorld->GetComponent<UITransformComponent>(
			useMuteIcon ? mMuteIconEntity : mTextEntity);
	UITextComponent* text =
		useMuteIcon ? nullptr : mWorld->GetComponent<UITextComponent>(mTextEntity);
	UISpriteComponent* muteIcon =
		useMuteIcon ? mWorld->GetComponent<UISpriteComponent>(mMuteIconEntity) : nullptr;
	if (!transform || (!text && !muteIcon))
		return;

	auto setAlpha = [&](float alpha)
	{
		if (text)
			text->mColor.f[3] = alpha;
		if (muteIcon)
			muteIcon->mColorTint.w = alpha;
	};

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
		setAlpha(progress);

		if (progress >= 1.0f)
		{
			mStage = PlayerStatusUIStage::Hold;
			mStageElapsed = 0.0f;
		}
		break;
	}
	case PlayerStatusUIStage::Hold:
		transform->mScale = Vec2(1.0f, 1.0f);
		setAlpha(1.0f);
		break;
	case PlayerStatusUIStage::Outro:
	{
		const float progress =
			std::clamp(mStageElapsed / max(spec.mOutroDuration, 0.001f), 0.0f, 1.0f);
		setAlpha(1.0f - progress);
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

				// 텍스트와 아이콘이 서로 전환될 때 새 표시 요소가 한 프레임 먼저 보이지 않도록 알파를 초기화
				if (mCurrentStatus == PlayerStatusUIType::Silenced)
				{
					if (UISpriteComponent* muteIcon =
						mWorld->GetComponent<UISpriteComponent>(mMuteIconEntity))
					{
						muteIcon->mColorTint.w = 0.0f;
					}
				}
				else if (UITextComponent* nextText =
					mWorld->GetComponent<UITextComponent>(mTextEntity))
				{
					nextText->mColor.f[3] = 0.0f;
				}
			}
		}
		break;
	}
	default:
		break;
	}
}

void PlayerStatusUIFeature::UpdateVisual(Entity player)
{
	if (mCurrentStatus == PlayerStatusUIType::None)
		return;

	if (mCurrentStatus == PlayerStatusUIType::Silenced)
	{
		UISpriteComponent* muteIcon =
			mWorld->GetComponent<UISpriteComponent>(mMuteIconEntity);
		if (!muteIcon)
			return;

		const int32 frameIndex = GetMuteIconFrameIndex(player);
		if (frameIndex < 0)
		{
			muteIcon->mVisible = false;
			return;
		}

		// \768 x 256 시트에서 로컬 캐릭터에 맞는 256 x 256 아이콘만 잘라 표시
		constexpr float muteIconSize = 256.0f;
		muteIcon->SetSourceRect(
			muteIconSize * static_cast<float>(frameIndex),
			0.0f,
			muteIconSize,
			muteIconSize);
		muteIcon->mVisible = true;

		if (UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity))
			text->mVisible = false;
		return;
	}

	UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity);
	if (!text)
		return;

	if (UISpriteComponent* muteIcon =
		mWorld->GetComponent<UISpriteComponent>(mMuteIconEntity))
	{
		muteIcon->mVisible = false;
	}
	text->mVisible = true;

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

int32 PlayerStatusUIFeature::GetMuteIconFrameIndex(Entity player) const
{
	const MainPlayerComponent* mainPlayer =
		mWorld->GetComponent<MainPlayerComponent>(player);
	if (!mainPlayer)
		return -1;

	// 시트의 아이콘 순서와 PlayerType 순서를 명시적으로 연결
	switch (mainPlayer->mPlayerType)
	{
	case PlayerType::Rudwig:
		return 0;
	case PlayerType::Ibanix:
		return 1;
	case PlayerType::Fanthor:
		return 2;
	default:
		return -1;
	}
}

void PlayerStatusUIFeature::EnsureUI()
{
	if (mTextEntity != NULL_ENTITY && mMuteIconEntity != NULL_ENTITY)
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

	mMuteIconEntity = mWorld->CreateEntity();

	auto& muteTransform =
		mWorld->AddComponent<UITransformComponent>(mMuteIconEntity);
	muteTransform.mAnchor = Anchor::Center;
	muteTransform.mPivot = Vec2(0.5f, 0.5f);
	muteTransform.mPosition = Vec2(0.0f, -150.0f);
	muteTransform.mSize = Vec2(128.0f, 128.0f);
	muteTransform.mUILayerIndex = 30;

	// 캐릭터별 뮤트 아이콘 시트를 사용하는 스프라이트를 생성
	auto& muteIcon = mWorld->AddComponent<UISpriteComponent>(
		mMuteIconEntity,
		RESOURCEMANAGER.Get<Texture>(L"UI_Player_Mute_Sheet"));
	muteIcon.mVisible = false;
	muteIcon.SetSourceRect(0.0f, 0.0f, 256.0f, 256.0f);
}

void PlayerStatusUIFeature::SetVisible(bool visible)
{
	if (UITextComponent* text = mWorld->GetComponent<UITextComponent>(mTextEntity))
	{
		text->mVisible =
			visible && mCurrentStatus != PlayerStatusUIType::Silenced;
	}

	if (UISpriteComponent* muteIcon =
		mWorld->GetComponent<UISpriteComponent>(mMuteIconEntity))
	{
		muteIcon->mVisible =
			visible && mCurrentStatus == PlayerStatusUIType::Silenced;
	}
}
