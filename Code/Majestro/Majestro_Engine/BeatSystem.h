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

private:
	float mBpm = 120.0f;
	int mBeat =0;
	float mSeconds = 0.0f;
};
