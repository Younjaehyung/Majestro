#pragma once

#include "Component.h"
#include "EnginePch.h"
#include "Entity.h"
#include "IMGUIComponent.h"

// 단일 Bullet 엔티티에서 다양한 탄종을 표현하기 위한 타입.
// 네트워크 전송 시 enum 값을 uint8로 직렬화해서 사용.
enum class SkillType : uint8
{
	Default = 0,
	BaseAttack,
	BaseSkill1,
	BaseSkill2,
	BaseSkill3,

	GuitarAttack,
	GuitarSkill1,
	GuitarSkill2,
	GuitarSkill3,

	DrumAttack,
	DrumSkill1,
	DrumSkill2,
	DrumSkill3,

	GuitarAttack_1,
	GuitarAttack_2,
	GuitarAttack_3,

	//mop
	HornAttack,

	Max
};


// 서버/로직 권한용 불릿 컴포넌트.
// 판정/수명/동기화에 필요한 데이터만 유지.
class BulletComponent : public Component<BulletComponent>
{
public:
	BulletComponent() = default;

	void Activate(
		SkillType type,
		uint64 ownerNetId,
		uint32 bulletNetId,
		uint16 generation,
		const Vec3& spawnPosition,
		const Vec3& direction,
		float speed,
		float lifeTime,
		float damage)
	{
		mType = type;
		mOwnerNetId = ownerNetId;
		mBulletNetId = bulletNetId;
		mGeneration = generation;

		mSpawnPosition = spawnPosition;
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
		mElapsedTime = 0.0f;
		mHitCount = 0;
		mIsActive = false;
	}

	void ResetForReuse()
	{
		mOwnerNetId = 0;
		mBulletNetId = 0;
		mGeneration = 0;
		mElapsedTime = 0.0f;
		mHitCount = 0;
		mIsActive = false;
	}

	bool UpdateLifeTime(float deltaTime)
	{
		mElapsedTime += deltaTime;
		return IsExpired();
	}

	bool IsExpired() const { return mElapsedTime >= mLifeTime; }
	bool CanHitMore() const { return mHitCount < mPenetrationCount; }
	void RegisterHit() { ++mHitCount; }

	void RegisterEditorProperties(std::vector<EditorProperty>& out)
	{
		out.push_back({ "Speed", PropertyType::Float, &mSpeed, 0.0f, 500.0f });
		out.push_back({ "Damage", PropertyType::Float, &mDamage, 0.0f, 500.0f });
		out.push_back({ "LifeTime", PropertyType::Float, &mLifeTime, 0.0f, 10.0f });
		out.push_back({ "Penetration", PropertyType::Int, &mPenetrationCount, 0.0f, 10.0f });
	}

public:
	// Server/Network (authority)
	SkillType mType = SkillType::Default;
	uint64 mOwnerNetId = 0;
	uint32 mBulletNetId = 0;
	uint16 mGeneration = 0;
	bool mReplicateOnActivate = true;
	bool mReplicateOnHit = true;

	// Combat
	float mDamage = 10.0f;
	float mSpeed = 60.0f;
	float mLifeTime = 1.5f;
	float mElapsedTime = 0.0f;
	int mPenetrationCount = 1;
	int mHitCount = 0;

	// Runtime
	Vec3 mSpawnPosition = {};
	Vec3 mDirection = Vec3::Forward;
	Vec3 mVelocity = {};
	bool mUseGravity = false;
	bool mUseHitscan = false;
};