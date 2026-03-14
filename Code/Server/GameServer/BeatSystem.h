#pragma once
#include "World.h"
#include "System.h"

class ControllerComponent;
class MainPlayerComponent;

class BeatSystem : public System
{
public:
	BeatSystem(World* world);

	void Initialize();
	void Update(float dt);

public:
	float mBpmSeconds = 60.f / mBpm;

private:
	int mBpm = 168.0f;
	int mBeat =0;

	float mSeconds = 0.0f;
	float mBonusTime = 0.2;
};
