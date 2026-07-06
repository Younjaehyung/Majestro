#pragma once
#include "Component.h"
#include "Entity.h"
#include "GameEvents.h"


struct BulletStat
{
	float Damage = 10.0f;
	float Speed = 90.0f;
	float Size = 0.25f;
	float LifeTime = 3.0f;
	float KnockbackDistance = 0.0f;
	bool  Penetrates = false;
	bool  PenetratesStatic = false;
};

inline BulletStat GetBulletStat(SkillType type)
{
	switch (type)
	{
	//case SkillType::DrumSkill1: return BulletStat{ 75.0f, 90.0f, 0.42f, 2.7f, 36.0f, 2 };
	//case SkillType::DrumSkill2: return BulletStat{ 0.0f, 70.0f, 0.60f, 3.2f, 55.0f, 1 };

		case SkillType::BaseAttack: return BulletStat{ 25.0f, 5300.0f, 0.5f, 6.0f, 0.0f, false };
		case SkillType::BaseAttack2: return BulletStat{ 10.0f, 5300.0f, 0.5f, 6.0f, 0.0f, false };
		case SkillType::BaseAttack3: return BulletStat{ 30.0f, 5300.0f, 0.5f, 6.0f, 0.0f, false };
		case SkillType::BaseSkill1: return BulletStat{ 75.0f, 1000.0f, 0.5f, 6.6f, 50.0f, true };
	//case SkillType::BaseSkill2: return BulletStat{ 0.0f, 80.0f, 0.50f, 3.0f, 40.0f, 1 };

		case SkillType::GuitarAttack_1: return BulletStat{ 30.0f, 4000.0f, 50.0f, 1.0f, 20.0f, true };
		case SkillType::GuitarAttack_2: return BulletStat{ 50.0f, 4000.0f, 50.0f, 0.7f, 32.0f, false };
		case SkillType::GuitarAttack_3: return BulletStat{ 70.0f, 4000.0f, 50.0f, 0.5f, 32.0f, false };


	case SkillType::HornAttack: return BulletStat{ 25.0f, 950.0f, 0.45f, 2.8f, 0.0f, false };
	case SkillType::BrassSkill2: return BulletStat{ 100.0f, 500.0f, 50.00f, 5.0f, 0.0f, false };
	case SkillType::BrassSkill3: return BulletStat{ 25.0f, 1100.0f, 0.40f, 2.2f, 0.0f, false };
	case SkillType::BrassSkill4: return BulletStat{ 75.0f, 900.0f, 100.0f, 20.0f, 0.0f, true, true };

	case SkillType::Default:
	default:
		return BulletStat{};
	}
}

// Bullet projectile runtime state.
class BulletComponent : public Component<BulletComponent>
{
public:
	SkillType mType = SkillType::Default;
	uint64 mOwnerNetId = 0;
	uint32 mBulletNetId = 0;
	uint16 mGeneration = 0;

	float mDamage = 10.0f;
	float mSpeed = 90.0f;
	float mLifeTime = 3.0f;
	float mKnockbackDistance = 0.0f;
	float mElapsedTime = 0.0f;
	bool mPenetrates = false;
	bool mPenetratesStatic = false;
	bool mIsCritical = false;

	Vec3 mDirection = Vec3::Forward;
	Vec3 mVelocity = {};
	bool mUseGravity = false;
	bool mUseHitscan = false;
	std::unordered_set<EntityID> mHitTargetIds;

	void Activate(SkillType type, uint64 ownerNetId, uint32 bulletNetId, uint16 generation,
		const Vec3& direction, float speed, float lifeTime, float damage, float knockbackDistance, bool isCritical = false)
	{
		mType = type;
		mOwnerNetId = ownerNetId;
		mBulletNetId = bulletNetId;
		mGeneration = generation;
		mDirection = direction;
		mSpeed = speed;
		mLifeTime = lifeTime;
		mDamage = damage;
		mKnockbackDistance = (std::max)(0.0f, knockbackDistance);
		mIsCritical = isCritical;
		mElapsedTime = 0.0f;
		mHitTargetIds.clear();
		mIsActive = true;
	}

	void Deactivate()
	{
		mIsActive = false;
		mElapsedTime = 0.0f;
		mPenetratesStatic = false;
		mIsCritical = false;
		mHitTargetIds.clear();
	}

	bool TryRegisterHitTarget(Entity target)
	{
		if (!target.IsValid())
			return false;

		return mHitTargetIds.insert(target.GetID()).second;
	}

	void UpdateLifeTime(float dt) { mElapsedTime += dt; }

	bool IsExpired() const { return (mElapsedTime >= mLifeTime && mLifeTime >= 0); }
};
