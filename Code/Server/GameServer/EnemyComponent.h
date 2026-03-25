#pragma once
#include "Component.h"
#include "Entity.h"
#include "GameTimer.h"

enum EnemyType {
	HornMan,
};

class EnemyComponent : public Component<EnemyComponent>
{
public:
	EnemyComponent() = default;
	explicit EnemyComponent(uint8 enemyType) : mEnemyType(enemyType) {}
	explicit EnemyComponent(uint8 enemyType, float speed) : mEnemyType(enemyType) , mSpeed(speed) {
	
		switch(mEnemyType){
		case EnemyType::HornMan:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		}
	}

public:
	uint8 mEnemyType = 0;
	float mSpeed;

	float AttackRange = 100;
	float AttackRangeSq = 100;

	float mAttackCool;
	float mNextAttackTime;


};