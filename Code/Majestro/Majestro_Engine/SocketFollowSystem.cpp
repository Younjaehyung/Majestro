#include "pch.h"
#include "SocketFollowSystem.h"

#include "AnimationComponent.h"
#include "PlayerComponent.h"
#include "SocketComponent.h"
#include "SocketSystem.h"
#include "TransformComponent.h"
#include "TransformSystem.h"
#include "VfxComponent.h"
#include "World.h"

SocketFollowSystem::SocketFollowSystem(World* world) :  System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> SocketFollowSystem::After() const
{
	return { typeid(TransformSystem), typeid(SocketSystem) };
}

void SocketFollowSystem::Update(float deltaTime)
{
	(void)deltaTime;

	if (mWorld == nullptr || mWorld->HasComponentPool<SocketFollowComponent>() == false)
		return;

	auto view = mWorld->View<SocketFollowComponent>();
	for (Entity entity : view)
	{
		SocketFollowComponent* follow = mWorld->GetComponent<SocketFollowComponent>(entity);
		if (follow == nullptr || follow->mIsActive == false)
			continue;

		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		if (transform == nullptr)
			continue;

		const Entity source = follow->mSourceEntity.IsValid() ? follow->mSourceEntity : entity;
		const bool active = IsFollowActive(source, *follow);

		Matrix socketMatrix = Matrix::Identity;
		bool socketValid = false;
		if (SocketComponent* sockets = mWorld->GetComponent<SocketComponent>(source))
			socketValid = sockets->TryGetSocketWorldMatrix(follow->mSocketName, socketMatrix);

		VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(entity);
		const bool keepFollowingStoppedVfx =
			active == false &&
			socketValid &&
			vfx != nullptr &&
			vfx->mUseWorldMatrix &&
			vfx->mStopRootWhenDisabled &&
			vfx->mIsPlaying &&
			vfx->efkHandle != -1;

		UpdateVfxPlayback(vfx, active && socketValid, *follow);

		if ((active || keepFollowingStoppedVfx) && socketValid)
		{

			ApplySocketTransform(*transform, socketMatrix, *follow);
		}
		else if (follow->mHideWhenInvalid)
		{
			SetHiddenTransform(*transform, *follow);
		}

		if (vfx != nullptr && vfx->mUseWorldMatrix)
		{
			
			vfx->mWorldMatrix = transform->mWorldMatrix;
		}
	}
}

bool SocketFollowSystem::IsFollowActive(Entity sourceEntity, const SocketFollowComponent& follow) const
{
	if (follow.mUseAttackState && IsAttackStateActive(sourceEntity) == false)
		return false;

	return IsAnimationWindowActive(sourceEntity, follow);
}

bool SocketFollowSystem::IsAttackStateActive(Entity sourceEntity) const
{
	const MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(sourceEntity);
	if (player == nullptr)
		return true;

	const int state = player->mUpperState;
	return state == static_cast<int>(ReplicatedActionState::Attack1) ||
		state == static_cast<int>(ReplicatedActionState::Attack2) ||
		state == static_cast<int>(ReplicatedActionState::Skill1) ||
		state == static_cast<int>(ReplicatedActionState::Skill2) ||
		state == static_cast<int>(ReplicatedActionState::Special);
}

bool SocketFollowSystem::IsAnimationWindowActive(Entity sourceEntity, const SocketFollowComponent& follow) const
{
	if (follow.mUseAnimationWindow == false)
		return true;

	const AnimationComponent* animation = mWorld->GetComponent<AnimationComponent>(sourceEntity);
	if (animation == nullptr || animation->mAnimClips.empty())
		return true;

	const bool useUpper = follow.mUseUpperAnimationWindow && animation->mEnableUpperBodyLayer;
	const uint32 clipIndex = useUpper ? animation->mUpperAnimClipIdx : animation->mLowerAnimClipIdx;
	if (clipIndex >= animation->mAnimClips.size())
		return true;

	const shared_ptr<Animator>& clip = animation->mAnimClips[clipIndex];
	if (clip == nullptr)
		return true;

	const float duration = max(static_cast<float>(clip->mDuration), 0.0001f);
	const float animTime = std::clamp(useUpper ? animation->mUpperUpdateTime : animation->mUpdateTime, 0.0f, duration);

	float startTime = max(follow.mTrailStartTime, 0.0f);
	float endTime = follow.mTrailEndTime;
	if (follow.mUseAnimationFrameWindow)
	{
		
		const uint32 frameCount = max(clip->mClipMeta.NumFrame, 1u);
		const float frameDuration = duration / static_cast<float>(frameCount);
		const uint32 startFrame = min(follow.mTrailStartFrame, frameCount - 1u);
		const uint32 endFrame = min(follow.mTrailEndFrame, frameCount - 1u);
		startTime = static_cast<float>(startFrame) * frameDuration;
		endTime = min(static_cast<float>(endFrame + 1u) * frameDuration, duration);
	}

	if (endTime <= startTime)
		endTime = duration;

	startTime = std::clamp(startTime, 0.0f, duration);
	endTime = std::clamp(endTime, 0.0f, duration);
	return animTime >= startTime && animTime < endTime;
}

void SocketFollowSystem::SetHiddenTransform(TransformComponent& transform, const SocketFollowComponent& follow) const
{
	transform.mLocalPosition = follow.mHiddenPosition;
	transform.mWorldPosition = follow.mHiddenPosition;
	transform.mLocalRotationE = Vec3::Zero;
	transform.mLocalRotationQ = Quaternion::Identity;
	transform.mLocalScale = Vec3(1.0f, 1.0f, 1.0f);
	transform.mLocalMatrix = Matrix::CreateTranslation(follow.mHiddenPosition);
	transform.mWorldMatrix = transform.mLocalMatrix;
}

void SocketFollowSystem::ApplySocketTransform(TransformComponent& transform, const Matrix& socketMatrix, const SocketFollowComponent& follow) const
{
	Matrix followMatrix = Matrix::CreateTranslation(follow.mSocketLocalOffset) * socketMatrix;

	Vec3 socketScale;
	Quaternion socketRotation;
	Vec3 socketPosition;
	followMatrix.Decompose(socketScale, socketRotation, socketPosition);
	socketPosition += follow.mWorldOffset;

	transform.mLocalPosition = socketPosition;
	transform.mWorldPosition = socketPosition;
	transform.mLocalScale = Vec3(1.0f, 1.0f, 1.0f);

	if (follow.mFollowRotation)
	{
		transform.mLocalRotationQ = socketRotation;
		transform.UpdateRotationEulerFromQuaternion();
		transform.mLocalMatrix = Matrix::CreateFromQuaternion(socketRotation) * Matrix::CreateTranslation(socketPosition);
	}
	else
	{
		transform.mLocalRotationE = Vec3::Zero;
		transform.mLocalRotationQ = Quaternion::Identity;
		transform.mLocalMatrix = Matrix::CreateTranslation(socketPosition);
	}

	
	transform.mWorldMatrix = transform.mLocalMatrix;
}

void SocketFollowSystem::UpdateVfxPlayback(VfxComponent* vfx, bool active, SocketFollowComponent& follow) const
{
	if (vfx == nullptr)
	{
		follow.mWasActive = active;
		return;
	}

	if (active)
	{
		if (follow.mRestartVfxOnActivate && follow.mWasActive == false)
		{
			
			vfx->efkHandle = -1;
			vfx->mIsPlaying = false;
			vfx->mTotalTime = 0.0f;
			vfx->mRootStopped = false;
		}

		vfx->mShouldPlay = true;
		vfx->mIsPaused = false;
		vfx->mFinished = false;
	}
	else
	{
		vfx->mShouldPlay = false;
	}

	follow.mWasActive = active;
}
