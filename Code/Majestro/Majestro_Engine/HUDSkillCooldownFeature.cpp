#include "pch.h"
#include "HUDSkillCooldownFeature.h"
#include "Engine.h"
#include "Timer.h"
#include "World.h"
#include "HUDSkillSlotComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"
#include "UISpriteComponent.h"
#include "GameEvents.h"


void HUDSkillCooldownFeature::Update(float /*dt*/)
{
	if (mWorld == nullptr || !mWorld->HasComponentPool<HUDSkillSlotComponent>())
		return;

	// 로컬 플레이어 찾기
	MainPlayerComponent* mp = nullptr;
	for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
	{
		if (mWorld->HasComponent<LocalPlayerComponent>(e))
		{
			mp = mWorld->GetComponent<MainPlayerComponent>(e);
			break;
		}
	}
	if (mp == nullptr)
		return;

	const float now = TIMER.GetTotalTime();

	for (Entity e : mWorld->GetEntitiesWithComponent<HUDSkillSlotComponent>())
	{
		auto* slot = mWorld->GetComponent<HUDSkillSlotComponent>(e);
		if (slot == nullptr || slot->mSkillSlot >= MainPlayerComponent::kCooldownSlotCount)
			continue;

		auto* ov = mWorld->GetComponent<UISpriteComponent>(slot->mOverlay);
		if (ov == nullptr)
			continue;

		const uint8 idx    = slot->mSkillSlot;
		const float dur    = mp->mCooldownDuration[idx];
		const float end    = mp->mCooldownEndLocal[idx];
		const float remain = end - now;
		const bool  cooling = (dur > 0.f && remain > 0.f);

		// 쿨다운 진행 
		if (mWasCooling[idx] && !cooling && (idx == 1 || idx == 2))
		{
			if (auto em = mWorld->GetEventManager())
				em->Enqueue(EvSfxRequest{ "player/Cooltime", Vec3::Zero });
		}
		mWasCooling[idx] = cooling;

		if (!cooling)   // 완료 덮개 숨김
		{
			ov->mVisible = false;
			continue;
		}

		const float ratio = std::clamp(remain / dur, 0.f, 1.f);  // 1=막 시작, 0=끝
		ov->mVisible = true;
		ov->SetVisibleRangeKeepDestinationSize(false);
		ov->SetVisibleRangeNormalizedY(0.f, ratio);			// 세로 크롭 (0,ratio) + 오버레이 pivot.y=0(상단 고정)
	}
}
