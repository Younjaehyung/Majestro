#include "pch.h"
#include "HUDSkillCooldownFeature.h"
#include "World.h"
#include "HUDSkillSlotComponent.h"
#include "PlayerComponent.h"
#include "TagComponent.h"
#include "UISpriteComponent.h"
#include "Timer.h"
#include <algorithm>

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

		const float dur    = mp->mCooldownDuration[slot->mSkillSlot];
		const float end    = mp->mCooldownEndLocal[slot->mSkillSlot];
		const float remain = end - now;

		if (dur <= 0.f || remain <= 0.f)   // 완료시 오버레이 숨김
		{
			ov->mVisible = false;
			continue;
		}

		const float ratio = std::clamp(remain / dur, 0.f, 1.f);  // 1=막 시작, 0=끝
		ov->mVisible = true;
		ov->SetVisibleRangeKeepDestinationSize(false);
		ov->SetVisibleRangeNormalized(0.f, ratio);   // 위에서 아래로 줄어드는 덮개
	}
}
