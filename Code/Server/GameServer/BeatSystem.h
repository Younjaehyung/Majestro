#pragma once
#include <vector>

#include "World.h"
#include "System.h"
#include "GameEvents.h"


class ControllerComponent;
class MainPlayerComponent;

class BeatSystem : public System
{
public:
	BeatSystem(World* world);

	void Initialize();
	void Update(float dt);

private:
	void CollectPendingBuffRequests();
	void ApplyPendingBuffRequests();

public:
	float mBpmSeconds = 60.f / mBpm;

private:
	int mBpm = 168.0f;
	int mBeat =0;

	float mSeconds = 0.0f;
	float mBonusTime = 0.2;
	std::vector<EvBuffRequest> mPendingBuffRequests;
};
