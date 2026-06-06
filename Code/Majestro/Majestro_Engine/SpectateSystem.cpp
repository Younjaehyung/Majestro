#include "pch.h"
#include "SpectateSystem.h"
#include "PlayerComponent.h"
#include "CameraComponent.h"
#include "DeathCamComponent.h"

SpectateSystem::SpectateSystem(World* world) : System(world)
{
}

void SpectateSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<DeathCamComponent>()) return;

	for (Entity entity : mWorld->GetEntitiesWithComponent<DeathCamComponent>())
	{
		DeathCamComponent*   death   = mWorld->GetComponent<DeathCamComponent>(entity);
		CameraTypeComponent* camType = mWorld->GetComponent<CameraTypeComponent>(entity);
		if (!death || !camType) continue;

		const int cycleReq = death->mSpectateCycleReq;
		death->mSpectateCycleReq = 0;

		if (!death->mActive) continue;
		if (death->mElapsed < death->mSpectateDelay) continue;


		// 관전 진입: 첫 대상 선택
		if (!death->mSpectating)
		{
			death->mSpectating     = true;
			death->mSpectateTarget = PickSpectateTarget(camType->mTargetID, death->mSpectateTarget, true);
		}

		// 다음/이전 순환
		if (cycleReq > 0)
			death->mSpectateTarget = PickSpectateTarget(camType->mTargetID, death->mSpectateTarget, true);
		else if (cycleReq < 0)
			death->mSpectateTarget = PickSpectateTarget(camType->mTargetID, death->mSpectateTarget, false);
	}
}

EntityID SpectateSystem::PickSpectateTarget(EntityID self, EntityID current, bool forward)
{
	std::vector<EntityID> candidates;
	for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
	{
		if (e.GetID() == self) continue;
		MainPlayerComponent* p = mWorld->GetComponent<MainPlayerComponent>(e);
		if (!p) continue;
		const bool dead =
			p->mLowerState == static_cast<int>(ReplicatedMovementMode::Dead) ||
			p->mUpperState == static_cast<int>(ReplicatedActionState::Dead);
		if (dead) continue;                       // 살아있는 팀원만
		candidates.push_back(e.GetID());
	}
	if (candidates.empty()) return 0;             // 대상 없으므로 자기 시신 유지

	auto it = std::find(candidates.begin(), candidates.end(), current);
	if (it == candidates.end()) return candidates.front();
	if (forward) { if (++it == candidates.end()) it = candidates.begin(); }
	else         { it = (it == candidates.begin()) ? candidates.end() - 1 : it - 1; }
	return *it;
}
