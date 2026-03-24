#pragma once
#include "Component.h"
#include "Entity.h"

enum EnemyType {
	HornMan,
};

class EnemyComponent : public Component<EnemyComponent>
{
public:
	EnemyComponent() = default;
	explicit EnemyComponent(uint8 enemyType) : mEnemyType(enemyType) {}
	explicit EnemyComponent(uint8 enemyType, float speed) : mEnemyType(enemyType) , mSpeed(speed) {}

	uint8 mEnemyType = 0;
	float mSpeed;
};