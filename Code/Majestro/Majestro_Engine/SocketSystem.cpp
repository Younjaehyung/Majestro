#include "pch.h"
#include "SocketSystem.h"

#include "SocketComponent.h"
#include "AnimationComponent.h"
#include "TransformComponent.h"
#include "CpuAnimationSystem.h"
#include "Skeleton.h"
#include "Animator.h"
#include "World.h"

SocketSystem::SocketSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> SocketSystem::After() const
{
	return {};
}

void SocketSystem::Update(float deltaTime)
{
	(void)deltaTime;

	if (mWorld == nullptr || mWorld->HasComponentPool<SocketComponent>() == false)
		return;


	CpuAnimationSystem* cpuAnim = mWorld->GetSystemManager()->GetSystem<CpuAnimationSystem>();

	auto view = mWorld->View<SocketComponent>();
	for (Entity entity : view)
	{
		SocketComponent* socketCom = mWorld->GetComponent<SocketComponent>(entity);
		TransformComponent* transformCom = mWorld->GetComponent<TransformComponent>(entity);
		if (socketCom == nullptr || transformCom == nullptr)
			continue;

		const Matrix worldMatrix = transformCom->GetWorldMatrix();


		shared_ptr<Skeleton> skeleton;
		if (AnimationComponent* animCom = mWorld->GetComponent<AnimationComponent>(entity))
		{
			if (animCom->mAnimClips.empty() == false)
			{
				const uint32 clipIdx = (animCom->mLowerAnimClipIdx < animCom->mAnimClips.size())
					? animCom->mLowerAnimClipIdx : 0u;
				if (animCom->mAnimClips[clipIdx] != nullptr)
					skeleton = animCom->mAnimClips[clipIdx]->GetSkeleton();
			}
		}

		for (SocketDef& socket : socketCom->mSockets)
		{
			// 본 이름이 비어 있으면 본에 붙지 않고 엔티티 월드행렬에 직접 부착
			if (socket.mBoneName.empty())
			{
				// row-major(v*M): socketWorld = socketLocal * entityWorld
				socket.mWorldMatrix = socket.mLocalOffset * worldMatrix;
				socket.mValid = true;
				continue;
			}

			// 본 인덱스 캐시 해석 / 스켈레톤이 있을 때만 시도
			if (socket.mCachedBoneIndex == Skeleton::INVALID_BONE_INDEX && skeleton != nullptr)
			{
				uint32 found = Skeleton::INVALID_BONE_INDEX;
				if (skeleton->TryFindBoneIndex(socket.mBoneName, found))
					socket.mCachedBoneIndex = found;
			}

			Matrix boneModel;
			if (cpuAnim != nullptr &&
				socket.mCachedBoneIndex != Skeleton::INVALID_BONE_INDEX &&
				cpuAnim->GetModelBoneMatrix(entity, socket.mCachedBoneIndex, boneModel))
			{
				socket.mWorldMatrix = socket.mLocalOffset * boneModel * worldMatrix;
				socket.mValid = true;
			}
			else
			{
				// 본 미해석/캐시 없음 — 이번 프레임 소켓 무효.
				socket.mValid = false;
			}
		}
	}
}
