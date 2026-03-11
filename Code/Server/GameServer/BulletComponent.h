#pragma once
#include "Component.h"
#include "Entity.h"

enum class BulletType : uint8
{
	Default = 0,
	BaseAttack,
	BaseSkill1,
	BaseSkill2,

	GuitarAttack,
	GuitarSkill1,
	GuitarSkill2,

	DrumAttack,
	DrumSkill1,
	DrumSkill2,
	Max
};

struct BulletStat
{
	float Damage = 10.0f;
	float Speed = 90.0f;
	float Size = 0.25f;
	float LifeTime = 3.0f;
	float KnockbackDistance = 0.0f;
	int PenetrationCount = 1;
	bool IsMeleeAttack = false;
};

inline BulletStat GetBulletStat(BulletType type)
{
	switch (type)
	{
	case BulletType::DrumAttack: return BulletStat{ 10.0f, 80.0f, 1000.20f, 1.12f, 12.0f, 1,true };
	case BulletType::DrumSkill1: return BulletStat{ 75.0f, 90.0f, 0.42f, 2.7f, 36.0f, 2 };
	case BulletType::DrumSkill2: return BulletStat{ 0.0f, 70.0f, 0.60f, 3.2f, 55.0f, 1 };

	case BulletType::BaseAttack: return BulletStat{ 15.0f, 130.0f, 0.22f, 2.0f, 0.0f, 1 };
	case BulletType::BaseSkill1: return BulletStat{ 75.0f, 100.0f, 0.35f, 2.6f, 50.0f, 2 };
	case BulletType::BaseSkill2: return BulletStat{ 0.0f, 80.0f, 0.50f, 3.0f, 40.0f, 1 };

	case BulletType::GuitarAttack: return BulletStat{ 25.0f, 150.0f, 0.20f, 1.8f, 10.0f, 1, true };
	case BulletType::GuitarSkill1: return BulletStat{ 30.0f, 120.0f, 0.30f, 2.4f, 20.0f, 2 };
	case BulletType::GuitarSkill2: return BulletStat{ 0.0f, 95.0f, 0.45f, 2.8f, 32.0f, 1 };

	case BulletType::Default:
	default:
		return BulletStat{};
	}
}

// 서버 권한 불릿 컴포넌트(예시)
class BulletComponent : public Component<BulletComponent>
{
public:
	BulletType mType = BulletType::Default;
	uint64 mOwnerNetId = 0;
	uint32 mBulletNetId = 0;
	uint16 mGeneration = 0;

	float mDamage = 10.0f;
	float mSpeed = 90.0f;
	float mLifeTime = 3.0f;
	float mKnockbackDistance = 0.0f;
	float mElapsedTime = 0.0f;
	int mPenetrationCount = 1;
	int mHitCount = 0;

	Vec3 mDirection = Vec3::Forward;
	Vec3 mVelocity = {};
	bool mUseGravity = false;
	bool mUseHitscan = false;
	bool mIsMeleeAttack = false;

	void Activate(BulletType type, uint64 ownerNetId, uint32 bulletNetId, uint16 generation,
		const Vec3& direction, float speed, float lifeTime, float damage, float knockbackDistance)
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
		mElapsedTime = 0.0f;
		mHitCount = 0;
		mIsActive = true;
		mIsMeleeAttack = false;
	}

	void Deactivate()
	{
		mIsActive = false;
		mElapsedTime = 0.0f;
		mHitCount = 0;
		mIsMeleeAttack = false;
	}

	bool UpdateLifeTime(float dt)
	{
		mElapsedTime += dt;
		return mElapsedTime >= mLifeTime;
	}
};