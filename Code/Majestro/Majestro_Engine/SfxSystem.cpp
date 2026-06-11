#include "pch.h"
#include "SfxSystem.h"

#include "Engine.h"
#include "World.h"
#include "GameEvents.h"
#include "JsonUtils.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"

SfxSystem::SfxSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

void SfxSystem::Initialize()
{
	LoadTable("../Resources/Json/SfxTable.json");
}

void SfxSystem::Update(float deltaTime)
{
	mTime += deltaTime;

	ConsumeEvents();
	PollPlayerStates();
}

void SfxSystem::OnWorldEnd()
{
	StopAllOwnedLoops();
}





void SfxSystem::LoadTable(const std::string& path)
{
	mTable.clear();

	std::ifstream ifs(path);
	if (!ifs)
	{
		std::cout << "[Sfx] SfxTable.json not found: " << path << std::endl;
		return;
	}

	json j;
	ifs >> j;

	mDefaultCooldown = GetOptionalFloat(j, "defaultCooldown", 0.05f);

	if (!j.contains("sfx") || !j["sfx"].is_object())
		return;

	for (auto it = j["sfx"].begin(); it != j["sfx"].end(); ++it)
	{
		const json& entry = it.value();
		if (!entry.is_object())
			continue;

		SfxDesc desc{};
		desc.eventPath = GetOptionalString(entry, "event");
		if (desc.eventPath.empty())
			continue;

		desc.is3d = GetOptionalBool(entry, "is3d", false);
		desc.loop = GetOptionalBool(entry, "loop", false);
		desc.cooldown = GetOptionalFloat(entry, "cooldown", mDefaultCooldown);
		mTable.emplace(it.key(), desc);
	}
}

const SfxDesc* SfxSystem::Find(const std::string& key)
{
	auto it = mTable.find(key);
	if (it == mTable.end())
	{
		if (mWarnedKeys.insert(key).second)
			std::cout << "[Sfx] unmapped key: " << key << std::endl;
		return nullptr;
	}
	return &it->second;
}

// 재생 
void SfxSystem::Play(const std::string& key, const Vec3* worldPos)
{
	const SfxDesc* desc = Find(key);
	if (desc == nullptr)
		return;

	auto last = mLastPlayTime.find(key);
	if (last != mLastPlayTime.end() && mTime - last->second < desc->cooldown)
		return;
	mLastPlayTime[key] = mTime;

	// FMOD 이벤트가 뱅크에 아직 없으면 FMOD_CHECK가 throw
	try
	{
		if (desc->is3d && worldPos != nullptr)
			AUDIOMANAGER.PlayOneShot3D(desc->eventPath.c_str(), MakeAttr(*worldPos));
		else
			AUDIOMANAGER.PlayOneShot(desc->eventPath.c_str());
	}
	catch (const std::exception& e)
	{
		if (mWarnedKeys.insert(desc->eventPath).second)
			std::cout << "[Sfx] play failed (" << desc->eventPath << "): " << e.what() << std::endl;
	}
}

void SfxSystem::HandleStateChange(SfxHandle& loopSlot, const std::string& key, const Vec3* pos)
{
	if (loopSlot != 0)
	{
		AUDIOMANAGER.StopLoop(loopSlot);
		loopSlot = 0;
	}

	const SfxDesc* desc = Find(key);
	if (desc == nullptr)
		return;

	if (!desc->loop)
	{
		Play(key, pos);
		return;
	}

	try
	{
		if (desc->is3d && pos != nullptr)
			loopSlot = AUDIOMANAGER.StartLoop3D(desc->eventPath.c_str(), MakeAttr(*pos));
		else
			loopSlot = AUDIOMANAGER.StartLoop(desc->eventPath.c_str());
	}
	catch (const std::exception& e)
	{
		if (mWarnedKeys.insert(desc->eventPath).second)
			std::cout << "[Sfx] loop start failed (" << desc->eventPath << "): " << e.what() << std::endl;
	}
}

// 이벤트 구독
void SfxSystem::ConsumeEvents()
{
	auto* eventManager = mWorld->GetEventManager().get();
	if (eventManager == nullptr)
		return;


	eventManager->Consume<EvVfxSpawnRequest>([this](const EvVfxSpawnRequest& e)
	{
		const std::string key = "skill/" + SkillTypeName(e.skillType) + "/" + SpawnReasonName(e.reason);
		Play(key, &e.position);
	});

	eventManager->Consume<EvHitMarker>([this](const EvHitMarker& e)
	{
		Play(e.isKill ? "ui/killmarker" : "ui/hitmarker");
	});

	eventManager->Consume<EvGamePhaseChanged>([this](const EvGamePhaseChanged& e)
	{
		Play("phase/" + std::to_string(static_cast<int>(e.newPhase)));
	});

	eventManager->Consume<EvHealthChanged>([this](const EvHealthChanged& e)
	{
		if (e.hp < e.previousHp)
			Play("player/hurt");
	});

	
	eventManager->Consume<EvSfxRequest>([this](const EvSfxRequest& e)
	{
		const bool has3dPosition = (e.position != Vec3::Zero);
		Play(e.sfxKey, has3dPosition ? &e.position : nullptr);
	});
}

// 플레이어 상태 폴링
void SfxSystem::PollPlayerStates()
{
	if (!mWorld->HasComponentPool<MainPlayerComponent>())
		return;

	std::unordered_set<EntityID> aliveEntities;

	for (Entity entity : mWorld->GetEntitiesWithComponent<MainPlayerComponent>())
	{
		MainPlayerComponent* player = mWorld->GetComponent<MainPlayerComponent>(entity);
		if (player == nullptr)
			continue;

		aliveEntities.insert(entity.GetID());

		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		const Vec3* position = transform ? &transform->mWorldPosition : nullptr;

		EntityLoopSlots& slot = mSlots[entity.GetID()];

		if (slot.seen && slot.upper != player->mUpperState)
		{
			const std::string key = "action/" + PlayerTypeName(player->mPlayerType)
			                      + "/" + ActionStateName(player->mUpperState);


			HandleStateChange(slot.upperLoop, key, position);
		}
		if (slot.seen && slot.lower != player->mLowerState)
		{
			const std::string key = "move/" + PlayerTypeName(player->mPlayerType)
			                      + "/" + MovementModeName(player->mLowerState);


			HandleStateChange(slot.lowerLoop, key, position);
		}

		slot.upper = player->mUpperState;
		slot.lower = player->mLowerState;
		slot.seen = true;

		// 활성 루프 매 프레임 위치 갱신
		if (position != nullptr && (slot.upperLoop != 0 || slot.lowerLoop != 0))
		{
			const FMOD_3D_ATTRIBUTES attr = MakeAttr(*position);
			if (slot.upperLoop != 0) AUDIOMANAGER.SetLoop3DAttributes(slot.upperLoop, attr);
			if (slot.lowerLoop != 0) AUDIOMANAGER.SetLoop3DAttributes(slot.lowerLoop, attr);
		}
	}

	// 사라진 엔티티(사망/디스폰)의 루프 정리
	for (auto it = mSlots.begin(); it != mSlots.end();)
	{
		if (aliveEntities.count(it->first) == 0)
		{
			if (it->second.upperLoop != 0) AUDIOMANAGER.StopLoop(it->second.upperLoop);
			if (it->second.lowerLoop != 0) AUDIOMANAGER.StopLoop(it->second.lowerLoop);
			it = mSlots.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SfxSystem::StopAllOwnedLoops()
{
	for (auto& [entityId, slot] : mSlots)
	{
		if (slot.upperLoop != 0) AUDIOMANAGER.StopLoop(slot.upperLoop);
		if (slot.lowerLoop != 0) AUDIOMANAGER.StopLoop(slot.lowerLoop);
	}
	mSlots.clear();
}

// 헬퍼
FMOD_3D_ATTRIBUTES SfxSystem::MakeAttr(const Vec3& position)
{
	FMOD_3D_ATTRIBUTES attr{};
	attr.position = { position.x, position.y, position.z };
	attr.forward = { 0.f, 0.f, 1.f };
	attr.up = { 0.f, 1.f, 0.f };
	return attr;
}

std::string SfxSystem::SkillTypeName(SkillType type)
{
	switch (type)
	{
	case SkillType::Default:        return "Default";
	case SkillType::BaseAttack:     return "BaseAttack";
	case SkillType::BaseSkill1:     return "BaseSkill1";
	case SkillType::BaseSkill2:     return "BaseSkill2";
	case SkillType::BaseSkill3:     return "BaseSkill3";
	case SkillType::GuitarAttack:   return "GuitarAttack";
	case SkillType::GuitarSkill1:   return "GuitarSkill1";
	case SkillType::GuitarSkill2:   return "GuitarSkill2";
	case SkillType::GuitarSkill3:   return "GuitarSkill3";
	case SkillType::DrumAttack:     return "DrumAttack";
	case SkillType::DrumSkill1:     return "DrumSkill1";
	case SkillType::DrumSkill2:     return "DrumSkill2";
	case SkillType::DrumSkill3:     return "DrumSkill3";
	case SkillType::GuitarAttack_1: return "GuitarAttack_1";
	case SkillType::GuitarAttack_2: return "GuitarAttack_2";
	case SkillType::GuitarAttack_3: return "GuitarAttack_3";
	case SkillType::HornAttack:     return "HornAttack";
	case SkillType::PianoAttack:    return "PianoAttack";
	case SkillType::BongoAttack:    return "BongoAttack";
	case SkillType::BongoShild:     return "BongoShild";
	default:                        return std::to_string(static_cast<int>(type));
	}
}

std::string SfxSystem::SpawnReasonName(uint8 reason)
{
	switch (static_cast<EffectSpawnReason>(reason))
	{
	case EffectSpawnReason::Fire:            return "Fire";
	case EffectSpawnReason::CollisionEntity: return "CollisionEntity";
	case EffectSpawnReason::CollisionStatic: return "CollisionStatic";
	case EffectSpawnReason::LifetimeExpired: return "LifetimeExpired";
	case EffectSpawnReason::Respawn:         return "Respawn";
	default:                                 return std::to_string(static_cast<int>(reason));
	}
}

std::string SfxSystem::PlayerTypeName(uint8 playerType)
{
	switch (static_cast<PlayerType>(playerType))
	{
	case PlayerType::Rudwig:  return "Rudwig";
	case PlayerType::Ibanix:  return "Ibanix";
	case PlayerType::Fanthor: return "Fanthor";
	default:                  return std::to_string(static_cast<int>(playerType));
	}
}

std::string SfxSystem::ActionStateName(int state)
{
	switch (static_cast<ReplicatedActionState>(state))
	{
	case ReplicatedActionState::None:         return "None";
	case ReplicatedActionState::Attack1:      return "Attack1";
	case ReplicatedActionState::Attack2:      return "Attack2";
	case ReplicatedActionState::ComboAttack1: return "ComboAttack1";
	case ReplicatedActionState::ComboAttack2: return "ComboAttack2";
	case ReplicatedActionState::Skill1:       return "Skill1";
	case ReplicatedActionState::Skill2:       return "Skill2";
	case ReplicatedActionState::Special:      return "Special";
	case ReplicatedActionState::Reload:       return "Reload";
	case ReplicatedActionState::RhythmChange: return "RhythmChange";
	case ReplicatedActionState::Aim:          return "Aim";
	case ReplicatedActionState::Hit:          return "Hit";
	case ReplicatedActionState::Stun:         return "Stun";
	case ReplicatedActionState::Dead:         return "Dead";
	default:                                  return std::to_string(state);
	}
}

std::string SfxSystem::MovementModeName(int state)
{
	switch (static_cast<ReplicatedMovementMode>(state))
	{
	case ReplicatedMovementMode::Idle:     return "Idle";
	case ReplicatedMovementMode::Grounded: return "Grounded";
	case ReplicatedMovementMode::Airborne: return "Airborne";
	case ReplicatedMovementMode::Falling:  return "Falling";
	case ReplicatedMovementMode::Landing:  return "Landing";
	case ReplicatedMovementMode::Dashing:  return "Dashing";
	case ReplicatedMovementMode::Disabled: return "Disabled";
	case ReplicatedMovementMode::Dead:     return "Dead";
	default:                               return std::to_string(state);
	}
}
