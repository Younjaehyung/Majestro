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
#include "HealthComponent.h"
#include <algorithm>
#include <array>
#include <vector>
#include <string>

namespace
{
	constexpr uint8 kPortraitSlotCount = 3;   // ROOM_MAX_PLAYERS 기준
}

const wchar_t* HUDPortraitUpdateFeature::PortraitPrefix(uint8 playerType)
{
	switch (static_cast<PlayerType>(playerType))
	{
	case PlayerType::Rudwig:  return L"Rudwig";
	case PlayerType::Ibanix:  return L"Ibanix";
	case PlayerType::Fanthor: return L"Fanthor";
	default:                  return nullptr;
	}
}

void HUDPortraitUpdateFeature::ApplyTextures(HUDPortraitSlotComponent& slot, uint8 playerType)
{
	const wchar_t* prefix = PortraitPrefix(playerType);
	if (prefix == nullptr)
		return;

	const std::wstring base = std::wstring(L"UI_") + prefix;

	auto setTex = [this](Entity e, const std::wstring& key)
	{
		if (auto* sp = mWorld->GetComponent<UISpriteComponent>(e))
			sp->mTexture = RESOURCEMANAGER.Get<Texture>(key);
	};

	setTex(slot.mBack0, base + L"_Portrait_0");
	setTex(slot.mBack1, base + L"_Portrait_1");
	setTex(slot.mHead0, base + L"_Portrait_Head_0");
	setTex(slot.mHead1, base + L"_Portrait_Head_1");
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

	const float ratio = std::clamp(
		static_cast<float>(health->mCurrentHp) / static_cast<float>(health->mMaxHp),
		0.0f, 1.0f);

	if (auto* back = mWorld->GetComponent<UISpriteComponent>(slot.mHpBack))
		back->mVisible = true;

	if (auto* fill = mWorld->GetComponent<UISpriteComponent>(slot.mHpFill))
	{
		fill->mVisible = true;
		fill->SetVisibleRangeKeepDestinationSize(false);   // HP 감소 시 바도 함께 줄어듦 (메인 바와 동일)
		fill->SetVisibleRangeNormalizedX(0.f, ratio);
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
