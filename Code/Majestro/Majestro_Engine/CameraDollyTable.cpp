#include "pch.h"
#include "CameraDollyTable.h"
#include "JsonUtils.h"

namespace
{
	// "캐릭터_상태" 형태의 이름 -> 프리셋 (예: "Rudwig_Skill1")
	std::unordered_map<std::string, DollyPreset> gNamedDollyTable;
}

void CameraDollyTable::Load(const std::string& path)
{
	gNamedDollyTable.clear();

	std::ifstream ifs(path);
	if (!ifs)
	{
		std::cout << "CameraDollySetting json not found: " << path << std::endl;
		return;
	}

	json root;
	ifs >> root;

	for (const auto& [characterName, states] : root.items())
	{
		for (const auto& [stateName, value] : states.items())
		{
			DollyPreset preset;
			preset.mDistance = GetOptionalFloat(value, "distance", 100.f);
			preset.mInSpeed  = GetOptionalFloat(value, "inSpeed", 12.f);
			preset.mOutSpeed = GetOptionalFloat(value, "outSpeed", 3.f);

			gNamedDollyTable.emplace(characterName + "_" + stateName, preset);
		}
	}
}

const DollyPreset* CameraDollyTable::Find(const std::string& presetName)
{
	const auto it = gNamedDollyTable.find(presetName);
	return it != gNamedDollyTable.end() ? &it->second : nullptr;
}
