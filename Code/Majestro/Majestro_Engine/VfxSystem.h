#pragma once
#include "System.h"

class TransformComponent;


struct VfxSpawnDesc
{
	const wchar_t* effectName = nullptr;
	Vec3 positionOffset = Vec3::Zero;
	Vec3 scale = Vec3(1.0f);
	bool followCaster = false;	// true면 시전자(casterNetId) 엔티티를 매 프레임 추종, false면 고정 월드 VFX
	bool followCasterRotation = false;	// true면 추종 중 시전자의 최신 조준 방향도 반영
	bool loopUntilSpecialEnds = false;
	float forwardOffset = 0.0f;	// 중앙 피벗 이펙트를 시전자 전방으로 이동
	bool loopUntilStopped = false;
};

struct BulletVfxDesc
{
	const wchar_t* effectName = nullptr;
	Vec3 scale = Vec3(1.0f);
};

// 균열 데칼
struct GroundDecalDesc
{
	const wchar_t* texName = nullptr;                 // 등록된 텍스처 키(없으면 링)
	float radius = 300.0f;                            // 데칼 반경(월드 단위)
	Vec4  color = Vec4(2.0f, 1.2f, 0.6f, 1.0f);       // >1 이면 Bloom 발광
	float lifetime = 1.5f;                            // 수명(초)
	Vec3  positionOffset = Vec3::Zero;                // 임팩트 위치 보정
	int   atlasCols = 1;                              // 아틀라스 열 수(1=전체)
	int   atlasRows = 1;                              // 아틀라스 행 수(1=전체)
	int   atlasIndex = 0;                             // 그릴 셀 인덱스(행 우선)
};

// 총알 데칼
struct BulletHoleDesc
{
	const wchar_t* texName = nullptr;                 // 등록된 텍스처 키(없으면 링)
	float size = 40.0f;                               // 구멍 half-size(월드 단위)
	Vec4  color = Vec4(0.1f, 0.1f, 0.1f, 0.9f);       // 그을음(어두운 색)
	float lifetime = 12.0f;                           // 수명(초). 누적 방지용 유한값 권장
	int   atlasCols = 1;                              // 아틀라스 열 수(1=전체)
	int   atlasRows = 1;                              // 아틀라스 행 수(1=전체)
	int   atlasIndex = 0;                             // 그릴 셀 인덱스(행 우선)
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
	void UpdateFollowTargets();
	void AttachBulletVfx(Entity bulletEntity, SkillType skillType, uint16 generation);
	Entity AcquirePooledEntity();
	void SetHiddenTransform(TransformComponent& transform);


	bool IsBaseSkill(SkillType type);
	bool IsRangedBulletSkill(SkillType type);
	bool IsStaticImpactSkill(SkillType type);

	std::optional<VfxSpawnDesc> ResolveVfxSpawn(SkillType skillType, uint8 reason);
	BulletVfxDesc ResolveBulletVfx(SkillType skillType);

	std::optional<GroundDecalDesc> ResolveGroundDecal(SkillType skillType, uint8 reason);

	// 총알 흔적
	std::optional<BulletHoleDesc> ResolveBulletHole(SkillType skillType, uint8 reason);

	// 근접 흔적
	std::optional<BulletHoleDesc> ResolveMeleeWallMark(SkillType skillType, uint8 reason);

private:
	static constexpr uint32 kPoolSize = 128;
	std::vector<Entity> mPool;
};
