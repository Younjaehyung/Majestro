#pragma once
#include "Component.h"
#include "Entity.h"

enum EnemyType {
	HornMan,
	Pianoman,
	Bongoman
};

enum class EnemyAnimState : uint8
{
	Run,
	Attack,
	Dead,
};

class EnemyComponent : public Component<EnemyComponent>
{
public:
	EnemyComponent() = default;
	explicit EnemyComponent(uint8 enemyType) : mEnemyType(enemyType) {}
	uint8 mEnemyType = 0;

	int mAnimState = static_cast<int>(EnemyAnimState::Run);
	float mDeadElapsedTime = 0.f;
};