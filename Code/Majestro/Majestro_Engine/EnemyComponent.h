#pragma once
#include "Component.h"
#include "Entity.h"

enum EnemyType {
	HornMan,
	Pianoman,
	Bongoman,
	Obelisk,
	Fly,
	Brass,
	Slime,
	Dragon
};


inline const wchar_t* BossAssetName(int enemyType)
{
	switch (enemyType)
	{
	case EnemyType::Brass:  return L"Tubaitan";
	case EnemyType::Dragon: return L"Violagon";
	default:                return L"Tubaitan";
	}
}

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
	BrassAttack4,
	DragonSkill1 = BrassAttack1,
	DragonSkill2 = BrassAttack2,
	DragonSkill3 = BrassAttack3,
	DragonSkill4 = BrassAttack4,

	BossSkill1 = BrassAttack1,
	BossSkill2 = BrassAttack2,
	BossSkill3 = BrassAttack3,
	BossSkill4 = BrassAttack4,
};


inline int BossSkillIndexOf(int animState)
{
	switch (static_cast<EnemyAnimState>(animState))
	{
	case EnemyAnimState::BossSkill1: return 1;
	case EnemyAnimState::BossSkill2: return 2;
	case EnemyAnimState::BossSkill3: return 3;
	case EnemyAnimState::BossSkill4: return 4;
	default:                        return 0;
	}
}

class EnemyComponent : public Component<EnemyComponent>
{
public:
	EnemyComponent() = default;
	explicit EnemyComponent(uint8 enemyType) : mEnemyType(enemyType) {}
	uint8 mEnemyType = 0;

	int mAnimState = static_cast<int>(EnemyAnimState::Run);
	float mDeadElapsedTime = 0.f;
};
