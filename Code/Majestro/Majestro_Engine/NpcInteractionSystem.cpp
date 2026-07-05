#include "pch.h"
#include "NpcInteractionSystem.h"
#include "NpcComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "InputManager.h"
#include "Engine.h"

namespace
{
	constexpr float kDialogueEyeHeight = 150.f;   // 플레이어/NPC 시선 높이
	constexpr float kDialogueAimSpeed = 8.f;      // 카메라 타겟팅 보간 속도 (1/s)

	// 대화 중 카메라(마우스 룩 yaw/pitch)를 NPC 방향으로 부드럽게 조준
	void AimCameraAtNpc(PlayerMovementComponent* movement,
		const Vec3& playerPos, const Vec3& npcPos, float dt)
	{
		Vec3 dir = (npcPos + Vec3(0.f, kDialogueEyeHeight, 0.f))
			- (playerPos + Vec3(0.f, kDialogueEyeHeight, 0.f));
		const float lenSq = dir.LengthSquared();
		if (lenSq <= 1.f)
			return;
		dir /= sqrtf(lenSq);

		const float targetYaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
		const float targetPitch = std::clamp(
			XMConvertToDegrees(-asinf(std::clamp(dir.y, -1.f, 1.f))), -60.f, 60.f);

		// yaw는 최단 각도로 wrap해서 보간
		float yawDiff = targetYaw - movement->mCurrentRotation.y;
		yawDiff = fmodf(yawDiff + 180.f, 360.f);
		if (yawDiff < 0.f)
			yawDiff += 360.f;
		yawDiff -= 180.f;

		const float alpha = std::clamp(dt * kDialogueAimSpeed, 0.f, 1.f);
		movement->mCurrentRotation.y += yawDiff * alpha;
		movement->mCurrentRotation.x = std::lerp(movement->mCurrentRotation.x, targetPitch, alpha);

		// 대화 종료 후 마우스 룩이 이어서 누적되도록 target도 동기화
		movement->mTargetRotation = movement->mCurrentRotation;
		movement->mCameraRotationX = movement->mCurrentRotation.x;
		movement->mCameraRotationY = movement->mCurrentRotation.y;
	}
}

NpcInteractionSystem::NpcInteractionSystem(World* world) : System(world)
{
	mPhase = SysPhase::Sim;
}

void NpcInteractionSystem::Update(float deltaTime)
{
	DialogueStateComponent* state = mWorld->GetSingleton<DialogueStateComponent>();
	if (!state)
		return;

	if (!mWorld->HasComponentPool<NpcComponent>())
	{
		state->mNearbyNpc = Entity{};
		return;
	}

	// 로컬 플레이어 위치
	Vec3 playerPos;
	bool hasPlayer = false;
	Entity playerEntity{};
	if (mWorld->HasComponentPool<LocalPlayerComponent>())
	{
		auto players = mWorld->GetEntitiesWithComponents<LocalPlayerComponent, TransformComponent>();
		if (!players.empty())
		{
			if (TransformComponent* tr = mWorld->GetComponent<TransformComponent>(players[0]))
			{
				playerPos = tr->mWorldPosition;
				playerEntity = players[0];
				hasPlayer = true;
			}
		}
	}

	if (!hasPlayer)
	{
		// 스폰 전/사망 등 — 대화 상태 정리
		state->mNearbyNpc = Entity{};
		state->mActive = false;
		state->mLineIndex = 0;
		return;
	}

	// 상호작용 반경 안의 가장 가까운 NPC
	Entity nearest{};
	float nearestDistSq = FLT_MAX;
	for (Entity e : mWorld->GetEntitiesWithComponent<NpcComponent>())
	{
		NpcComponent* npc = mWorld->GetComponent<NpcComponent>(e);
		TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);
		if (!npc || !tr) continue;

		const float distSq = Vec3::DistanceSquared(playerPos, tr->mWorldPosition);
		if (distSq <= npc->mInteractRadius * npc->mInteractRadius && distSq < nearestDistSq)
		{
			nearestDistSq = distSq;
			nearest = e;
		}
	}
	state->mNearbyNpc = nearest;

	// 대화 진행 중: F 로 다음 대사, 마지막 대사 이후 종료
	if (state->mActive)
	{
		NpcComponent* npc = mWorld->GetComponent<NpcComponent>(state->mNpc);
		if (!npc)
		{
			state->mActive = false;
			state->mLineIndex = 0;
			return;
		}

		// 카메라를 NPC 방향으로 타겟팅
		PlayerMovementComponent* movement = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
		TransformComponent* npcTr = mWorld->GetComponent<TransformComponent>(state->mNpc);
		if (movement && npcTr)
			AimCameraAtNpc(movement, playerPos, npcTr->mWorldPosition, deltaTime);

		if (INPUT.GetKeyDown(eKeyCode::F))
		{
			++state->mLineIndex;
			if (state->mLineIndex >= (int32)npc->mDialogueLines.size())
			{
				state->mActive = false;
				state->mLineIndex = 0;
			}
		}
		return;
	}

	// 대화 시작
	if (nearest.IsValid() && INPUT.GetKeyDown(eKeyCode::F))
	{
		NpcComponent* npc = mWorld->GetComponent<NpcComponent>(nearest);
		if (npc && !npc->mDialogueLines.empty())
		{
			state->mActive = true;
			state->mNpc = nearest;
			state->mLineIndex = 0;
		}
	}
}
