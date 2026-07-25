#include "pch.h"
#include "HUDSkillCooldownFeature.h"
#include "Engine.h"
#include "Timer.h"
#include "World.h"
#include "HUDSkillSlotComponent.h"
#include "PlayerComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "UITransformComponent.h"
#include "BeatSystem.h"
#include "MathUtils.h"
#include "GameEvents.h"

namespace
{
	constexpr float kPopDuration = 0.28f;   // 준비 완료 플래시 지속(초)
}

void HUDSkillCooldownFeature::Update(float dt)
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


	float beatBounce = 1.f;
	if (auto systemManager = mWorld->GetSystemManager())
	{
		if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
		{
			if (beatSystem->GetBeatSeconds() > 0.f)
				beatBounce = MathUtils::BeatBounceScale(beatSystem->GetBeatProgress(), kUIBeatBounceAmplitude);
		}
	}

	for (Entity e : mWorld->GetEntitiesWithComponent<HUDSkillSlotComponent>())
	{
		auto* slot = mWorld->GetComponent<HUDSkillSlotComponent>(e);
		if (slot == nullptr)
			continue;

		// --- 슬롯 진행도 계산 ---
		bool  ready      = true;   // 사용 가능(쿨 완료 / 충전 만땅)
		float coverTop   = 0.f;    // 덮개가 위에서부터 차지하는 비율 (0=없음, 1=꽉 덮음)
		bool  showNumber = false;
		int   number     = 0;

		if (slot->mIsUltimate)
		{
			const float maxPts = static_cast<float>(MainPlayerComponent::kMaxRhythmPoints);
			const float charge = maxPts > 0.f
				? std::clamp(static_cast<float>(mp->mRhythmPoints) / maxPts, 0.f, 1.f)
				: 0.f;
			ready    = (charge >= 0.999f);
			coverTop = 1.f - charge;   // 충전될수록 덮개가 아래로 줄어 바텀업 필
		}
		else
		{
			if (slot->mSkillSlot >= MainPlayerComponent::kCooldownSlotCount)
				continue;
			const uint8 idx    = slot->mSkillSlot;
			const float dur    = mp->mCooldownDuration[idx];
			const float end    = mp->mCooldownEndLocal[idx];
			const float remain = end - now;
			const bool  cooling = (dur > 0.f && remain > 0.f);

			ready      = !cooling;
			coverTop   = cooling ? std::clamp(remain / dur, 0.f, 1.f) : 0.f;
			showNumber = cooling;
			number     = static_cast<int>(std::ceil(remain));
		}

		// --- 준비 완료 에지: 플래시 + 사운드 ---
		if (ready && !slot->mPrevReady)
		{
			slot->mPopTimer = kPopDuration;
			if (auto em = mWorld->GetEventManager())
				em->Enqueue(EvSfxRequest{ "player/Cooltime", Vec3::Zero });
		}
		slot->mPrevReady = ready;
		if (slot->mPopTimer > 0.f)
			slot->mPopTimer = (std::max)(0.f, slot->mPopTimer - dt);

		// --- 덮개(쿨다운 와이프 / 충전 필) ---
		if (auto* ov = mWorld->GetComponent<UISpriteComponent>(slot->mOverlay))
		{
			if (coverTop <= 0.001f)
			{
				ov->mVisible = false;
			}
			else
			{
				ov->mVisible = true;
				ov->SetVisibleRangeKeepDestinationSize(false);
				ov->SetVisibleRangeNormalizedY(0.f, coverTop);  // pivot.y=0(상단) 기준 위에서부터 덮음
			}
		}

		// --- 아이콘 tint: 준비 플래시 > 스킬 쿨 디새추레이션 > 풀컬러 ---
		// (궁극기는 충전 덮개가 진행을 보여주므로 디새추레이션 안 함 — 컬러 유지)
		if (auto* ic = mWorld->GetComponent<UISpriteComponent>(slot->mIcon))
		{
			if (ready && slot->mPopTimer > 0.f)
			{
				const float p = slot->mPopTimer / kPopDuration;         // 1→0
				const float b = 1.f + 0.8f * p;                         // 준비 순간 번쩍 → 원복
				ic->mColorTint = Vec4(b, b, b, 1.f);
			}
			else if (!ready && !slot->mIsUltimate)
			{
				ic->mColorTint = Vec4(0.42f, 0.42f, 0.50f, 1.f);        // 스킬 쿨: 회색+어둠
			}
			else
			{
				ic->mColorTint = Vec4(1.f, 1.f, 1.f, 1.f);              // 풀컬러(궁 충전중 포함)
			}
		}

		// --- 남은 초 숫자 (스킬만) ---
		if (slot->mCountText != NULL_ENTITY)
		{
			if (auto* tx = mWorld->GetComponent<UITextComponent>(slot->mCountText))
			{
				if (showNumber)
				{
					tx->mVisible = true;
					tx->mText = std::to_wstring((std::max)(0, number));

					if (auto* tr = mWorld->GetComponent<UITransformComponent>(slot->mCountText))
						tr->mScale = Vec2(beatBounce, beatBounce);
				}
				else
				{
					tx->mVisible = false;
				}
			}
		}

		// --- 궁극기 만충 글로우 (back 패널 금빛 펄스) ---
		if (slot->mIsUltimate)
		{
			if (auto* bk = mWorld->GetComponent<UISpriteComponent>(slot->mBack))
			{
				if (ready)
				{
					const float pulse = 0.5f + 0.5f * std::sin(now * 6.5f); // 0..1
					const float g = 0.85f + 0.45f * pulse;                  // 0.85..1.3
					bk->mColorTint = Vec4(1.25f * g, 1.10f * g, 0.55f * g, 1.f); // 금빛
				}
				else
				{
					bk->mColorTint = Vec4(1.f, 1.f, 1.f, 1.f);
				}
			}
		}
	}
}
