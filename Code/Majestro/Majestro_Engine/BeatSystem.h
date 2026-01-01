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
	int mBpm = 120;
	int mBeat =0;
	float mSeconds = 0.0f;
};
