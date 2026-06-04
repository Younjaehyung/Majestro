#pragma once
#include "AnimationComponent.h"
#include "PlayerComponent.h"
#include "MovementComponent.h"
#include "NetTransformComponent.h"
#include "TransformComponent.h"

enum class ClientAnimState : uint16
{
	Idle = 0,
	RunForward,
	RunBackward,
	RunRight,
	RunLeft,
	Jump,
	Fall,
	Land,
	Dash,
	Attack1,
	Attack2,
	ComboAttack1,
	ComboAttack2,
	Skill1,
	Skill2,
	Special,
	Reload,
	RhythmChange,
	Aim,
	Dead,
	Hit,
	Stun,
};

struct PlayerAnimationResolveResult
{
	uint32 LowerClipIndex = 0;
	uint32 UpperClipIndex = 0;
	bool EnableUpperLayer = false;
};

inline ClientAnimState ToClientActionState(int actionState)
{
	switch (static_cast<ReplicatedActionState>(actionState))
	{
	case ReplicatedActionState::Attack1: return ClientAnimState::Attack1;
	case ReplicatedActionState::Attack2: return ClientAnimState::Attack2;
	case ReplicatedActionState::ComboAttack1: return ClientAnimState::ComboAttack1;
	case ReplicatedActionState::ComboAttack2: return ClientAnimState::ComboAttack2;
	case ReplicatedActionState::Skill1: return ClientAnimState::Skill1;
	case ReplicatedActionState::Skill2: return ClientAnimState::Skill2;
	case ReplicatedActionState::Special: return ClientAnimState::Special;
	case ReplicatedActionState::Reload: return ClientAnimState::Reload;
	case ReplicatedActionState::RhythmChange: return ClientAnimState::RhythmChange;
	case ReplicatedActionState::Aim: return ClientAnimState::Aim;
	case ReplicatedActionState::Hit: return ClientAnimState::Hit;
	case ReplicatedActionState::Stun: return ClientAnimState::Stun;
	case ReplicatedActionState::Dead: return ClientAnimState::Dead;
	case ReplicatedActionState::None:
	default:
		return ClientAnimState::Idle;
	}
}

inline ClientAnimState SelectDirectionalMoveState(float localX, float localZ)
{
	if (std::abs(localZ) >= std::abs(localX))
		return localZ >= 0.f ? ClientAnimState::RunForward : ClientAnimState::RunBackward;

	return localX >= 0.f ? ClientAnimState::RunRight : ClientAnimState::RunLeft;
}

inline ClientAnimState ResolveLowerByMovement(
	int movementModeValue,
	int controlFlags,
	int externalMoveModeValue,
	TransformComponent* transform,
	PlayerMovementComponent* movement,
	NetTransformComponent* netTransform)
{
	const ReplicatedMovementMode movementMode = static_cast<ReplicatedMovementMode>(movementModeValue);
	const ReplicatedExternalMoveMode externalMoveMode = static_cast<ReplicatedExternalMoveMode>(externalMoveModeValue);
	const bool canControlHorizontal = (controlFlags & Control_CanControlHorizontal) != 0;
	switch (movementMode)
	{
	case ReplicatedMovementMode::Dead:
		return ClientAnimState::Dead;
	case ReplicatedMovementMode::Dashing:
		return ClientAnimState::Dash;
	case ReplicatedMovementMode::Falling:
		return ClientAnimState::Fall;
	case ReplicatedMovementMode::Landing:
		return ClientAnimState::Land;
	case ReplicatedMovementMode::Airborne:
		return ClientAnimState::Jump;
	case ReplicatedMovementMode::Disabled:
		return ClientAnimState::Idle;
	case ReplicatedMovementMode::Idle:
		return ClientAnimState::Idle;
	case ReplicatedMovementMode::Grounded:
		break;
	default:
		break;
	}

	constexpr float kInputThresholdSq = 0.0001f;
	constexpr float kVelocityThresholdSq = 1.0f;

	if (canControlHorizontal && movement && movement->mMovingDirection.LengthSquared() > kInputThresholdSq)
	{
		return SelectDirectionalMoveState(movement->mMovingDirection.x, movement->mMovingDirection.z);
	}

	const bool canUseServerVelocity =
		externalMoveMode != ReplicatedExternalMoveMode::OverrideXZ &&
		externalMoveMode != ReplicatedExternalMoveMode::OverrideAll;
	if (canUseServerVelocity && netTransform && netTransform->mVelocity.LengthSquared() > kVelocityThresholdSq && transform)
	{
		Vec3 velocity = netTransform->mVelocity;
		velocity.y = 0.f;
		if (velocity.LengthSquared() > kVelocityThresholdSq)
		{
			velocity.Normalize();
			const float localX = velocity.Dot(transform->GetRight());
			const float localZ = velocity.Dot(transform->GetLook());
			return SelectDirectionalMoveState(localX, localZ);
		}
	}

	return ClientAnimState::Idle;
}

inline uint32 GetDefaultClipIndex(ClientAnimState state)
{
	switch (state)
	{
	case ClientAnimState::RunForward: return 1;
	case ClientAnimState::RunBackward: return 2;
	case ClientAnimState::RunRight: return 3;
	case ClientAnimState::RunLeft: return 4;
	case ClientAnimState::Jump: return 5;
	case ClientAnimState::Fall: return 6;
	case ClientAnimState::Land: return 7;
	case ClientAnimState::Dash: return 8;
	case ClientAnimState::Attack1: return 9;
	case ClientAnimState::Attack2: return 10;
	case ClientAnimState::ComboAttack1: return 11;
	case ClientAnimState::ComboAttack2: return 12;
	case ClientAnimState::Skill1: return 13;
	case ClientAnimState::Skill2: return 14;
	case ClientAnimState::Special: return 15;
	case ClientAnimState::Reload: return 16;
	case ClientAnimState::RhythmChange: return 17;
	case ClientAnimState::Aim: return 18;
	case ClientAnimState::Dead: return 19;
	case ClientAnimState::Idle:
	case ClientAnimState::Hit:
	case ClientAnimState::Stun:
	default:
		return 0;
	}
}

inline bool IsUpperBodyAction(ClientAnimState state)
{
	switch (state)
	{
	case ClientAnimState::Aim:
	case ClientAnimState::Attack1:
	case ClientAnimState::Attack2:
	case ClientAnimState::ComboAttack1:
	case ClientAnimState::ComboAttack2:
	case ClientAnimState::Special:
	case ClientAnimState::Reload:
	case ClientAnimState::RhythmChange:
		return true;
	default:
		return false;
	}
}

inline bool IsFullBodyState(ClientAnimState state)
{
	switch (state)
	{
	case ClientAnimState::Dash:
	case ClientAnimState::Jump:
	case ClientAnimState::Fall:
	case ClientAnimState::Land:
	case ClientAnimState::Hit:
	case ClientAnimState::Stun:
	case ClientAnimState::Dead:
	case ClientAnimState::Skill1:
	case ClientAnimState::Skill2:
		return true;
	default:
		return false;
	}
}

inline uint32 ResolveClipIndex(const AnimationComponent& animCom, ClientAnimState state, uint32 fallback)
{
	const uint32 index = GetDefaultClipIndex(state);
	if (index < animCom.mAnimClips.size() && animCom.mAnimClips[index])
		return index;

	if (fallback < animCom.mAnimClips.size() && animCom.mAnimClips[fallback])
		return fallback;

	return 0;
}

inline PlayerAnimationResolveResult ResolvePlayerAnimationState(
	const MainPlayerComponent& player,
	const AnimationComponent& animCom,
	TransformComponent* transform,
	PlayerMovementComponent* movement,
	NetTransformComponent* netTransform)
{
	PlayerAnimationResolveResult result{};

	// FSM split: server packets carry action/movement meaning, not directional animation clips.
	const ClientAnimState lowerState = ResolveLowerByMovement(
		player.mLowerState,
		player.mControlFlags,
		player.mExternalMoveMode,
		transform,
		movement,
		netTransform);
	const ClientAnimState actionState = ToClientActionState(player.mUpperState);
	const bool fullBodyAction = IsFullBodyState(actionState);
	const ClientAnimState upperState = fullBodyAction ? actionState : actionState;

	result.LowerClipIndex = ResolveClipIndex(animCom, fullBodyAction ? actionState : lowerState, 0);
	result.UpperClipIndex = ResolveClipIndex(animCom, upperState, result.LowerClipIndex);

	const bool hasUpperAction = IsUpperBodyAction(upperState) && !IsFullBodyState(lowerState) && !fullBodyAction;
	result.EnableUpperLayer = hasUpperAction && result.UpperClipIndex != result.LowerClipIndex;

	return result;
}
