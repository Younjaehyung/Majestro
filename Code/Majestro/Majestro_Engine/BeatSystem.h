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
	int mBpm = 167;
	int mBeat = 0;
	int mLastFiredBeat = -1; // 마지막으로 이벤트를 발행한 박자 번호

	float mBpmSeconds = 60.f / mBpm;
	float mSeconds = 0.0f;
	float mBonusTime = 0.2f;
};
