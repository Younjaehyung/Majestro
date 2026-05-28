#pragma once
#include "World.h"
#include "System.h"
#include <vector>
class EnemySystem :public System
{
public:
	EnemySystem(World* world);

	void Initialize() {};
	void Update(float deltaTime);

private:
	struct AttackDebugIndicator
	{
		Vec3 center = Vec3::Zero;
		float radius = 0.0f;
		float remainingTime = 0.0f;
		Vec4 color = Vec4::One;
		bool isPlayerAttack = false;
	};

	void UpdateAttackDebugIndicators(float dt);

	std::vector<AttackDebugIndicator> mAttackDebugIndicators;
};

