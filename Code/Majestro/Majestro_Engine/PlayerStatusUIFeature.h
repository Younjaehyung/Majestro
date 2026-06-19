#pragma once
#include "UIFeature.h"
#include "Entity.h"

enum class PlayerStatusUIType : uint8
{
	None,
	Silenced,
	Stunned,
	Dead
};

enum class PlayerStatusUIStage : uint8
{
	Idle,
	Intro,
	Hold,
	Outro
};

struct PlayerStatusUISpec
{
	std::wstring mTitle;
	int32 mPriority = 0;
	float mIntroDuration = 0.18f;
	float mOutroDuration = 0.2f;
	float mIntroStartScale = 1.35f;
	Vec4 mColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

class PlayerStatusUIFeature : public UIFeature
{
public:
	void Initialize(World* world) override;
	void Update(float dt) override;

private:
	Entity FindLocalPlayer() const;
	PlayerStatusUIType SelectStatus(Entity player) const;
	float GetRemaining(Entity player, PlayerStatusUIType type) const;
	void ChangeStatus(PlayerStatusUIType nextStatus);
	void TickAnimation(float dt);
	void UpdateVisual(Entity player);
	int32 GetMuteIconFrameIndex(Entity player) const;
	void EnsureUI();
	void SetVisible(bool visible);

private:
	std::unordered_map<PlayerStatusUIType, PlayerStatusUISpec> mSpecs;
	Entity mTextEntity = NULL_ENTITY;
	Entity mMuteIconEntity = NULL_ENTITY;
	PlayerStatusUIType mCurrentStatus = PlayerStatusUIType::None;
	PlayerStatusUIType mPendingStatus = PlayerStatusUIType::None;
	PlayerStatusUIStage mStage = PlayerStatusUIStage::Idle;
	float mStageElapsed = 0.0f;
};
