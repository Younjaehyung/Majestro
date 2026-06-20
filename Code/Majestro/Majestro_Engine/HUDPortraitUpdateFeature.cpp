#include "pch.h"
#include "HUDPortraitUpdateFeature.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "World.h"
#include "Texture.h"
#include "HUDPortraitSlotComponent.h"
#include "PlayerComponent.h"   
#include "NetEntityComponent.h"
#include "TagComponent.h"      
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "UITransformComponent.h"
#include "UIComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include <algorithm>
#include <array>
#include <vector>
#include <string>

namespace
{
	constexpr float kPortraitCellSize = 768.0f;
	constexpr uint8 kPortraitSlotCount = 3;   // ROOM_MAX_PLAYERS 기준
}

int32 HUDPortraitUpdateFeature::PortraitAtlasRow(uint8 playerType)
{
	switch (static_cast<PlayerType>(playerType))
	{
	case PlayerType::Ibanix:  return 0;
	case PlayerType::Rudwig:  return 1;
	case PlayerType::Fanthor: return 2;
	default:                  return -1;
	}
}

void HUDPortraitUpdateFeature::ApplyTextures(HUDPortraitSlotComponent& slot, uint8 playerType)
{
	const int32 row = PortraitAtlasRow(playerType);
	if (row < 0)
		return;

	const shared_ptr<Texture> atlas =
		RESOURCEMANAGER.Get<Texture>(L"UI_Ingame_Sheet");
	if (atlas == nullptr)
		return;

	// 초상화 영역은 768 픽셀 정사각형 셀이다.
	// 열 순서는 Head_0, Head_1, Portrait_0, Portrait_1이다.
	auto applyCell = [this, &atlas, row](Entity entity, int32 column)
	{
		if (UISpriteComponent* sprite =
			mWorld->GetComponent<UISpriteComponent>(entity))
		{
			sprite->mTexture = atlas;
			sprite->SetSourceRect(
				kPortraitCellSize * static_cast<float>(column),
				kPortraitCellSize * static_cast<float>(row),
				kPortraitCellSize,
				kPortraitCellSize);
		}
	};

	applyCell(slot.mBack0, 2);
	applyCell(slot.mBack1, 3);
	applyCell(slot.mHead0, 0);
	applyCell(slot.mHead1, 1);
}

void HUDPortraitUpdateFeature::SetSlotVisible(HUDPortraitSlotComponent& slot, bool visible)
{
	const Entity sprites[4] = { slot.mBack0, slot.mBack1, slot.mHead0, slot.mHead1 };
	for (Entity e : sprites)
		if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mVisible = visible;
}

void HUDPortraitUpdateFeature::HideHpBar(HUDPortraitSlotComponent& slot)
{
	if (slot.mHpFill == NULL_ENTITY)   // slot0(로컬)은 미니 HP 바가 없음
		return;
	if (auto* sp = mWorld->GetComponent<UISpriteComponent>(slot.mHpBack)) sp->mVisible = false;
	if (auto* sp = mWorld->GetComponent<UISpriteComponent>(slot.mHpFill)) sp->mVisible = false;
	if (auto* sp = mWorld->GetComponent<UISpriteComponent>(slot.mHpShield)) sp->mVisible = false;
	if (auto* tx = mWorld->GetComponent<UITextComponent>(slot.mHpText))   tx->mVisible = false;
}

void HUDPortraitUpdateFeature::UpdateHpBar(HUDPortraitSlotComponent& slot, Entity owner)
{
	// slot0(로컬)은 상단 메인 HP 바를 쓰므로 미니 바 핸들이 없다.
	if (slot.mHpFill == NULL_ENTITY)
		return;

	HealthComponent* health = mWorld->GetComponent<HealthComponent>(owner);
	if (health == nullptr || health->mMaxHp <= 0)
	{
		HideHpBar(slot);
		return;
	}

	// 상용게임 오버실드 방식: 바 전체 스케일 denom = max(MaxHp, 현재체력+쉴드).
	// 체력+쉴드 <= MaxHp 면 denom=MaxHp 고정(체력 비율 유지), 넘칠 때만 비율 재조정.
	ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(owner);
	const int32 shield = (armor != nullptr) ? (std::max)(0, armor->mCurrentArmor) : 0;
	const int32 curHp = (std::max)(0, health->mCurrentHp);
	const float denom = static_cast<float>((std::max)(health->mMaxHp, curHp + shield));

	const float ratio = std::clamp(
		static_cast<float>(curHp) / denom,
		0.0f, 1.0f);

	if (auto* back = mWorld->GetComponent<UISpriteComponent>(slot.mHpBack))
		back->mVisible = true;

	if (auto* fill = mWorld->GetComponent<UISpriteComponent>(slot.mHpFill))
	{
		fill->mVisible = true;
		fill->SetVisibleRangeKeepDestinationSize(false);   // HP 감소 시 바도 함께 줄어듦 (메인 바와 동일)
		fill->SetVisibleRangeNormalizedX(0.f, RemapBarRatioToUv(ratio, mHpFillUvRangeX));
	}

	// 쉴드 바: "현재 체력" 바로 오른쪽 [currentHp/denom, (currentHp+쉴드)/denom] 구간을 채운다.
	if (auto* shieldSp = mWorld->GetComponent<UISpriteComponent>(slot.mHpShield))
	{
		UITransformComponent* fillTr = mWorld->GetComponent<UITransformComponent>(slot.mHpFill);
		UITransformComponent* shieldTr = mWorld->GetComponent<UITransformComponent>(slot.mHpShield);
		if (shield > 0 && fillTr != nullptr && shieldTr != nullptr)
		{
			// HP fill 트랜스폼 + UV 구간에서 실제 바 영역(디자인 좌표) 도출.
			const float barLeftX = fillTr->mPosition.x + fillTr->mSize.x * mHpFillUvRangeX.x;
			const float barWidth = fillTr->mSize.x * (mHpFillUvRangeX.y - mHpFillUvRangeX.x);
			// 미니 바는 Y UV 구간을 안 쓰므로 메인 바와 동일한 116~140/256 사용.
			const float barTopY = fillTr->mPosition.y + fillTr->mSize.y * (116.f / 256.f);
			const float barHeight = fillTr->mSize.y * (24.f / 256.f);

			const float segLeftFrac = static_cast<float>((std::max)(0, health->mCurrentHp)) / denom;
			const float segWidthFrac = static_cast<float>(shield) / denom;

			shieldTr->mPosition = Vec2(barLeftX + barWidth * segLeftFrac, barTopY);
			shieldTr->mSize = Vec2(barWidth * segWidthFrac, barHeight);
			shieldSp->mVisible = true;
		}
		else
		{
			shieldSp->mVisible = false;
		}
	}

	if (auto* text = mWorld->GetComponent<UITextComponent>(slot.mHpText))
	{
		text->mVisible = true;
		text->mText = std::to_wstring(health->mCurrentHp) + L"/" + std::to_wstring(health->mMaxHp);
	}
}

void HUDPortraitUpdateFeature::Update(float /*dt*/)
{
	if (mWorld == nullptr || !mWorld->HasComponentPool<HUDPortraitSlotComponent>())
		return;

	// 1) 현재 World 의 플레이어 수집
	struct PlayerInfo { uint64 netId; uint8 type; bool local; Entity entity; };
	std::vector<PlayerInfo> players;
	for (Entity e : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
	{
		auto* mp = mWorld->GetComponent<MainPlayerComponent>(e);
		if (mp == nullptr)
			continue;

		auto* net = mWorld->GetComponent<NetEntityComponent>(e);
		const uint64 netId = net ? net->mNetEntityId : 0;
		const bool   local = mWorld->HasComponent<LocalPlayerComponent>(e);
		players.push_back({ netId, static_cast<uint8>(mp->mPlayerType), local, e });
	}

	// 2) 슬롯 배정: slot0 = 로컬, slot1.. = 원격(netId 오름차순 = 접속 순서)
	std::array<int16, kPortraitSlotCount>  slotType;    // -1 = 빈칸
	std::array<Entity, kPortraitSlotCount> slotOwner;   // 슬롯에 배정된 플레이어 엔티티
	slotType.fill(-1);
	slotOwner.fill(NULL_ENTITY);

	for (const auto& p : players)
		if (p.local) { slotType[0] = p.type; slotOwner[0] = p.entity; break; }

	std::vector<PlayerInfo> remotes;
	for (const auto& p : players)
		if (!p.local) remotes.push_back(p);
	std::sort(remotes.begin(), remotes.end(),
		[](const PlayerInfo& a, const PlayerInfo& b) { return a.netId < b.netId; });

	for (size_t i = 0; i < remotes.size() && (i + 1) < kPortraitSlotCount; ++i)
	{
		slotType[i + 1]  = remotes[i].type;
		slotOwner[i + 1] = remotes[i].entity;
	}

	// 3) 슬롯 컴포넌트 갱신
	for (Entity e : mWorld->GetEntitiesWithComponent<HUDPortraitSlotComponent>())
	{
		auto* slot = mWorld->GetComponent<HUDPortraitSlotComponent>(e);
		if (slot == nullptr || slot->mSlotIndex >= kPortraitSlotCount)
			continue;

		const int16  type  = slotType[slot->mSlotIndex];
		const Entity owner = slotOwner[slot->mSlotIndex];
		slot->mOwnerEntity = owner;

		if (type < 0)   // 빈 슬롯 → 초상화·HP 바 모두 숨김
		{
			SetSlotVisible(*slot, false);
			HideHpBar(*slot);
			slot->mOccupied = false;
			slot->mCurrentType = 0xFF;
			continue;
		}

		if (slot->mCurrentType != static_cast<uint8>(type))   // 변경 시에만 텍스처 재설정
		{
			ApplyTextures(*slot, static_cast<uint8>(type));
			slot->mCurrentType = static_cast<uint8>(type);
		}
		SetSlotVisible(*slot, true);
		UpdateHpBar(*slot, owner);   // slot0 은 핸들이 없어 내부에서 즉시 반환
		slot->mOccupied = true;
	}
}
