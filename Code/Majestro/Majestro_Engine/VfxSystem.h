#pragma once
#include "System.h"

class TransformComponent;


struct VfxSpawnDesc
{
	const wchar_t* effectName = nullptr;
	Vec3 positionOffset = Vec3::Zero;
	Vec3 scale = Vec3(1.0f);
};

struct BulletVfxDesc
{
	const wchar_t* effectName = nullptr;
	Vec3 scale = Vec3(1.0f);
};

class VfxSystem : public System
{
public:
	VfxSystem(World* world);

	void Initialize() override;
	void Update(float deltaTime) override;

	Entity PlayOneShot(const wstring& effectName, const Vec3& position, const Vec3& rotation, const Vec3& scale);

private:
	void CreatePool();
	void ConsumeSpawnEvents();
	void ReturnFinishedEffects();
	void AttachBulletVfx(Entity bulletEntity, SkillType skillType, uint16 generation);
	Entity AcquirePooledEntity();
	void SetHiddenTransform(TransformComponent& transform);


	bool IsBaseSkill(SkillType type);
	bool IsRangedBulletSkill(SkillType type);
	bool IsStaticImpactSkill(SkillType type);

	std::optional<VfxSpawnDesc> ResolveVfxSpawn(SkillType skillType, uint8 reason);
	BulletVfxDesc ResolveBulletVfx(SkillType skillType);

private:
	static constexpr uint32 kPoolSize = 128;
	std::vector<Entity> mPool;
};
