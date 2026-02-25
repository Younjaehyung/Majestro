#pragma once
#include "Component.h"
#include "Entity.h"

enum class BulletType : uint8
{
	Default = 0,
	Rifle,
	Shotgun,
	Rocket,
	Laser,
	Max
};

// 서버 권한 불릿 컴포넌트(예시)
class BulletComponent : public Component<BulletComponent>
{
public:
	BulletType mType = BulletType::Default;
	uint64 mOwnerNetId = 0;
	uint32 mBulletNetId = 0;
	uint16 mGeneration = 0;

	float mDamage = 10.0f;
	float mSpeed = 60.0f;
	float mLifeTime = 1.5f;
	float mElapsedTime = 0.0f;
	int mPenetrationCount = 1;
	int mHitCount = 0;

	Vec3 mDirection = Vec3::Forward;
	Vec3 mVelocity = {};
	bool mUseGravity = false;
	bool mUseHitscan = false;

	void Activate(BulletType type, uint64 ownerNetId, uint32 bulletNetId, uint16 generation,
		const Vec3& direction, float speed, float lifeTime, float damage)
	{
		mType = type;
		mOwnerNetId = ownerNetId;
		mBulletNetId = bulletNetId;
		mGeneration = generation;
		mDirection = direction;
		mSpeed = speed;
		mLifeTime = lifeTime;
		mDamage = damage;
		mElapsedTime = 0.0f;
		mHitCount = 0;
		mIsActive = true;
	}

	void Deactivate()
	{
		mIsActive = false;
		mElapsedTime = 0.0f;
		mHitCount = 0;
	}

	bool UpdateLifeTime(float dt)
	{
		mElapsedTime += dt;
		return mElapsedTime >= mLifeTime;
	}
};