#pragma once
#include "Component.h"
#include "Entity.h"
#include "GameTimer.h"

enum EnemyType {
	HornMan,
	Pianoman,
	Bongoman
};

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
	explicit EnemyComponent(uint8 enemyType, float speed) : mEnemyType(enemyType) , mSpeed(speed) {
	
		switch(mEnemyType){
		case EnemyType::HornMan:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		case EnemyType::Pianoman:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		case EnemyType::Bongoman:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		default:
			mAttackCool = 4;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		}

	}

public:
	uint8 mEnemyType = 0;
	uint8 mAnimState;
	float mSpeed;

	float AttackRange = 100;
	float AttackRangeSq = 100;

	float mAttackCool;
	float mNextAttackTime;
	float mPendingAttackTime = -1.0f;

	float mAttackAnimEndTime = 0.0f;
	float mAttackAnimTime = 1.0f;
};