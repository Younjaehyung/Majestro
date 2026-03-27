#pragma once
#include "Component.h"
#include "Entity.h"

enum class EnemyAnimState : uint8
{
	Run = 0,
	Attack = 1,
	Dead = 2,
};

class EnemyComponent : public Component<EnemyComponent>
{
public:
	EnemyComponent() = default;
	explicit EnemyComponent(uint8 enemyType) : mEnemyType(enemyType) {}
	uint8 mEnemyType = 0;

	int mAnimStatePacket;
	float mDeadElapsedTime = 0.f;
};