#pragma once
#include "World.h"
#include "System.h"

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

public:
	const float mDPI = 5.f;

	const float sensitivity = 0.1f;
	const float lerpFactor = 15.0f; // 값이 클수록 즉각적, 작을수록 부드러움
	const float deadzone = 0.001f;  // 미세 떨림 무시 임계값
};