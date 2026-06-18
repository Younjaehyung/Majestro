#pragma once
#include "Component.h"
#include <algorithm>
#include <array>

class PlayerStatusComponent : public Component<PlayerStatusComponent>
{
public:
	uint8 mStatusFlags = PlayerStatus_None;
	float mStunRemaining = 0.0f;
	float mRespawnRemaining = 0.0f;
	uint8 mBuffCount = 0;
	std::array<ReplicatedBuffState, MAX_REPLICATED_BUFFS> mBuffs{};

	const ReplicatedBuffState* FindBuff(ReplicatedBuffType type) const
	{
		for (uint8 index = 0; index < mBuffCount && index < MAX_REPLICATED_BUFFS; ++index)
		{
			if (mBuffs[index].buffType == type)
				return &mBuffs[index];
		}
		return nullptr;
	}

	float GetBuffRemaining(ReplicatedBuffType type) const
	{
		float longestRemaining = 0.0f;
		bool found = false;

		for (uint8 index = 0; index < mBuffCount && index < MAX_REPLICATED_BUFFS; ++index)
		{
			const ReplicatedBuffState& buff = mBuffs[index];
			if (buff.buffType != type)
				continue;

			found = true;
			if (buff.remainingTime < 0.0f)
				return -1.0f;

			longestRemaining = std::max(longestRemaining, buff.remainingTime);
		}

		return found ? longestRemaining : 0.0f;
	}
};
