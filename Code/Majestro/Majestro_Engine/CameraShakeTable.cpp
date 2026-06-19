#include "pch.h"
#include "CameraShakeTable.h"
#include "PlayerComponent.h"
#include "JsonUtils.h"


namespace
{
	constexpr size_t kPlayerTypeCount  = static_cast<size_t>(PlayerType::Count);
	constexpr size_t kActionStateCount = static_cast<size_t>(ReplicatedActionState::Count);

	// [PlayerType][ActionState]
	std::array<std::array<std::optional<ShakePreset>, kActionStateCount>, kPlayerTypeCount> gShakeTable{};
	std::unordered_map<std::string, ShakePreset> gNamedShakeTable;

	// JSON 문자열 키 → enum 매핑
	const std::unordered_map<std::string, PlayerType> kPlayerTypeByName = {
		{ "Rudwig",  PlayerType::Rudwig  },
		{ "Ibanix",  PlayerType::Ibanix  },
		{ "Fanthor", PlayerType::Fanthor },
	};

	const std::unordered_map<std::string, ReplicatedActionState> kActionStateByName = {
		{ "Attack1",      ReplicatedActionState::Attack1      },
		{ "Attack2",      ReplicatedActionState::Attack2      },
		{ "ComboAttack1", ReplicatedActionState::ComboAttack1 },
		{ "ComboAttack2", ReplicatedActionState::ComboAttack2 },
		{ "Skill1",       ReplicatedActionState::Skill1       },
		{ "Skill2",       ReplicatedActionState::Skill2       },
		{ "Special",      ReplicatedActionState::Special      },
		{ "Rythm",        ReplicatedActionState::RhythmChange       },
		{ "Reload",       ReplicatedActionState::Reload       },
		{ "Special",      ReplicatedActionState::Special      },
	};
}

void CameraShakeTable::Load(const std::string& path)
{
	for (auto& row : gShakeTable)
		for (auto& slot : row)
			slot.reset();
	gNamedShakeTable.clear();

	std::ifstream ifs(path);
	if (!ifs)
	{
		std::cout << "CameraShakeSetting json not found: " << path << std::endl;
		return;
	}

	json root;
	ifs >> root;

	for (const auto& [characterName, states] : root.items())
	{
		auto typeIt = kPlayerTypeByName.find(characterName);
		if (typeIt == kPlayerTypeByName.end())
		{
			std::cout << "CameraShakeSetting: 알 수 없는 캐릭터 키 " << characterName << std::endl;
			continue;
		}

		for (const auto& [stateName, value] : states.items())
		{
			auto stateIt = kActionStateByName.find(stateName);
			if (stateIt == kActionStateByName.end())
			{
				std::cout << "CameraShakeSetting: 알 수 없는 상태 키 " << stateName << std::endl;
				continue;
			}

			ShakePreset preset;
			preset.mAngles.x  = GetOptionalFloat(value, "pitch");
			preset.mAngles.y  = GetOptionalFloat(value, "yaw");
			preset.mAngles.z  = GetOptionalFloat(value, "roll");
			preset.mFrequency = GetOptionalFloat(value, "frequency", 20.f);

			gShakeTable[static_cast<size_t>(typeIt->second)][static_cast<size_t>(stateIt->second)] = preset;
			gNamedShakeTable.emplace(characterName + "_" + stateName, preset);
		}
	}
}

const ShakePreset* CameraShakeTable::Find(PlayerType playerType, ReplicatedActionState state)
{
	const size_t typeIdx  = static_cast<size_t>(playerType);
	const size_t stateIdx = static_cast<size_t>(state);

	if (typeIdx >= kPlayerTypeCount || stateIdx >= kActionStateCount)
		return nullptr;

	const auto& slot = gShakeTable[typeIdx][stateIdx];
	return slot.has_value() ? &slot.value() : nullptr;
}

const ShakePreset* CameraShakeTable::Find(const std::string& presetName)
{
	const auto it = gNamedShakeTable.find(presetName);
	return it != gNamedShakeTable.end() ? &it->second : nullptr;
}
