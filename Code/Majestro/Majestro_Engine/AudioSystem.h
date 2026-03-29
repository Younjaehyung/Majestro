#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "AudioManager.h"

class AudioSystem : public System
{
public:
	AudioSystem(World* world);

	void Initialize();
	void Update(float);
	void Shutdown();
private:
	float time{};
	void ApplyRhythmLayerByPlayerType(uint8 playerType, uint8 rhythm);
	bool IsCurrentRhythmMatched(uint8 playerType, uint8 rhythm) const;
	static SOUNDNAME GetSoundNameByPlayerType(uint8 playerType);
	static const char* GetExpectedMarkerByPlayerType(uint8 playerType, uint8 rhythm);
};

