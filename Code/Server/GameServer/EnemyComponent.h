#pragma once
#include "Component.h"
#include "Entity.h"
#include "GameTimer.h"

enum EnemyType {
	HornMan,
	Pianoman,
	Bongoman,
	Obelisk,
	Fly,
	Brass
};

enum class EnemyAnimState : uint8
{
	Run,
	Attack,
	Dead,
	Shield,
	RushEnd,
	BrassAttack1,
	BrassAttack2,
	BrassAttack3,
	BrassAttack4
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
			mNextUtilityTime = GetServerTotalTimeSeconds();
			break;
		case EnemyType::Obelisk:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			mSpeed = 0.0f;
			mNextUtilityTime = GetServerTotalTimeSeconds();
			mUtilityIntervalBeats = 4.0f;
			mUtilityAmount = 10;
			break;
		case EnemyType::Fly:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			mHoverHeight = 250.0f;
			break;
		case EnemyType::Brass:
			mAttackCool = 16;
			AttackRange = 700.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			mBrassAttackCool[0] = 4.0f;
			mBrassAttackCool[1] = 1.0f;
			mBrassAttackCool[2] = 4.0f;
			mBrassAttackCool[3] = 4.0f;
			for (int i = 0; i < 4; ++i)
				mBrassNextAttackTime[i] = GetServerTotalTimeSeconds();
			break;
		case EnemyType::Pianoman:
			mAttackCool = 16;
			AttackRange = 1000.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		case EnemyType::Bongoman:
			mAttackCool = 16;
			AttackRange = 300.f;
			AttackRangeSq = AttackRange * AttackRange;
			mNextAttackTime = GetServerTotalTimeSeconds();
			mNextShildTime = GetServerTotalTimeSeconds();
			break;
		default:
			mAttackCool = 4;
			mNextAttackTime = GetServerTotalTimeSeconds();
			break;
		}

	}

public:
	uint8 mEnemyType = 0;
	uint8 mAnimState = static_cast<int>(EnemyAnimState::Run);
	float mSpeed = 10.f;

	float AttackRange = 100.f;
	float AttackRangeSq = 100.f;

	float mAttackCool = 10.f;
	float mNextAttackTime = 5.f;
	float mNextShildTime = 0.0f;
	float mPendingAttackTime = -1.0f;
	uint8 mPendingSkillType = 0;
	bool mPianoRushVfxPlayed = false;

	float mAttackAnimEndTime = 0.0f;
	float mAttackAnimTime = 1.0f;
	float mShieldAnimEndTime = 0.0f;
	float mShieldAnimTime = 1.0f;
	float mRushEndAnimEndTime = 0.0f;
	float mRushEndAnimTime = 0.45f;
	float mNextUtilityTime = 0.0f;
	float mUtilityIntervalBeats = 0.0f;
	int32 mUtilityAmount = 0;
	float mHoverHeight = 0.0f;
	Entity mLinkedPlayer{};
	uint8 mBrassAttackPattern = 0;
	float mBrassAttackCool[4] = { 0.f, 0.f, 0.f, 0.f };
	float mBrassNextAttackTime[4] = { 0.f, 0.f, 0.f, 0.f };
	uint8 mBrassSkill3ShotsRemaining = 0;
	float mBrassSkill3NextShotTime = 0.0f;
	float mBrassSkill3ShotInterval = 0.0f;
	float mBrassRushEndHoldUntilTime = 0.0f;
	bool mBossEncounterActivated = false;
};
