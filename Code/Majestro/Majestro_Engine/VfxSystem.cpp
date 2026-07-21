#include "pch.h"
#include "VfxSystem.h"


#include "Engine.h"
#include "ResourceManager.h"

#include "TransformComponent.h"
#include "MovementComponent.h"
#include "TagComponent.h"
#include "RenderSystem.h"
#include "PlayerComponent.h"
#include "BulletComponent.h"
#include "VfxComponent.h"
#include "World.h"
#include "Vfx.h"
#include "GameEvents.h"
#include "DecalFactory.h"
#include "MathUtils.h"

namespace
{
	constexpr float kBaseUltimateCollisionLength = 5000.0f;
	constexpr float kBaseUltimateCollisionRadius = 100.0f;

	void SubmitDebugOrientedCylinder(
		const Vec3& start,
		const Vec3& direction,
		float length,
		float radius,
		const Vec4& color)
	{
		if (length <= 0.0f || radius <= 0.0f || direction.LengthSquared() <= 1e-6f)
			return;

		Vec3 axis = direction;
		axis.Normalize();
		Vec3 reference = std::abs(axis.Dot(Vec3::Up)) > 0.99f ? Vec3::Right : Vec3::Up;
		Vec3 right = reference.Cross(axis);
		right.Normalize();
		Vec3 up = axis.Cross(right);
		up.Normalize();

		constexpr int kSegments = 16;
		constexpr int kRings = 5;
		for (int ring = 0; ring < kRings; ++ring)
		{
			const float distance = length * static_cast<float>(ring) / static_cast<float>(kRings - 1);
			const Vec3 center = start + axis * distance;
			for (int segment = 0; segment < kSegments; ++segment)
			{
				const float angle0 = DirectX::XM_2PI * static_cast<float>(segment) / static_cast<float>(kSegments);
				const float angle1 = DirectX::XM_2PI * static_cast<float>(segment + 1) / static_cast<float>(kSegments);
				const Vec3 point0 = center + (right * std::cos(angle0) + up * std::sin(angle0)) * radius;
				const Vec3 point1 = center + (right * std::cos(angle1) + up * std::sin(angle1)) * radius;
				RenderSystem::SubmitDebugLine(point0, point1, color);
			}
		}

		const Vec3 end = start + axis * length;
		for (int segment = 0; segment < kSegments; ++segment)
		{
			const float angle = DirectX::XM_2PI * static_cast<float>(segment) / static_cast<float>(kSegments);
			const Vec3 offset = (right * std::cos(angle) + up * std::sin(angle)) * radius;
			RenderSystem::SubmitDebugLine(start + offset, end + offset, color);
		}
	}

	constexpr int kVfxDecalCols            = 4;   // 열 수
	constexpr int kVfxDecalRows            = 1;   // 행 수
	constexpr int kCellGroundCrack         = 0;   // 지면 균열
	constexpr int kCellBulletHoleDefault   = 1;   // 총알 흔적
	constexpr int kCellBulletHoleGuitar    = 2;   // 기타 평타 흔적
	constexpr int kCellBulletHoleDrum      = 3;   // 드럼 평타 흔적

	int PickBulletHoleCell(SkillType type)
	{
		switch (type)
		{
		case SkillType::GuitarAttack:
		case SkillType::GuitarAttack_1:
		case SkillType::GuitarAttack_2:
		case SkillType::GuitarAttack_3:
			return kCellBulletHoleGuitar;

		case SkillType::DrumAttack:
		case SkillType::DrumAttack3:
			return kCellBulletHoleDrum;

		default:
			return kCellBulletHoleDefault;
		}
	}

	// 근접 평타(투사체 없음)
	bool IsMeleeWallMarkAttack(SkillType type)
	{
		switch (type)
		{
		case SkillType::GuitarAttack:
		case SkillType::DrumAttack:
		case SkillType::DrumAttack3:
		case SkillType::BongoAttack:
		case SkillType::SlimeAttack:
		case SkillType::FlyAttack:
			return true;
		default:
			return false;
		}
	}

	constexpr float kMeleeWeaponHeight = 90.0f;  // 무기 높이
	constexpr float kMeleeReachBack    = 50.0f;  // 뒤 여유
	constexpr float kMeleeReachForward = 500.0f; // 앞(사거리)
	constexpr float kMeleeMarkSize     = 90.0f;  // 흔적 크기
}

VfxSystem::VfxSystem(World* world)
	: System(world)
{
	mPhase = SysPhase::Post;
}

void VfxSystem::Initialize()
{
	CreatePool();
}

void VfxSystem::Update(float deltaTime)
{
	(void)deltaTime;

	ReturnFinishedEffects();
	ConsumeSpawnEvents();
	UpdateFollowTargets();
}

Entity VfxSystem::PlayOneShot(const wstring& effectName, const Vec3& position, const Vec3& rotation, const Vec3& scale)
{
	shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(effectName);
	if (vfx == nullptr)
	{
#ifdef _DEBUG
		std::wcout << L"[VfxSystem] missing vfx resource: " << effectName << std::endl;
#endif
		return Entity{};
	}

	Entity entity = AcquirePooledEntity();
	if (!entity.IsValid())
	{
#ifdef _DEBUG
		std::wcout << L"[VfxSystem] vfx pool exhausted: " << effectName << std::endl;
#endif
		return Entity{};
	}

	TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
	VfxComponent* component = mWorld->GetComponent<VfxComponent>(entity);
	if (transform == nullptr || component == nullptr)
		return Entity{};

	// 원샷 VFX 대여 시 재생 전에 위치와 회전부터 적용해 이전 풀 위치가 한 프레임 보이지 않게 한다.
	transform->mLocalPosition = position;
	transform->mWorldPosition = position;
	transform->mLocalRotationE = rotation;
	transform->mWorldMatrix = Matrix::CreateTranslation(position);

	component->mIsPooled = true;
	component->mAutoReturn = true;
	component->ResetForPoolPlay(vfx, scale, false);
	return entity;
}

void VfxSystem::CreatePool()
{
	mPool.reserve(kPoolSize);

	for (uint32 i = 0; i < kPoolSize; ++i)
	{
		Entity entity = mWorld->CreateEntity();

		TransformComponent transform{};
		SetHiddenTransform(transform);
		mWorld->AddComponent<TransformComponent>(entity, transform);

		VfxComponent& component = mWorld->AddComponent<VfxComponent>(entity);
		component.mIsPooled = true;
		component.mAutoReturn = true;
		component.ResetForPoolIdle();
		component.mIsPooled = true;
		component.mAutoReturn = true;

		mPool.push_back(entity);
	}
}

void VfxSystem::ConsumeSpawnEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (eventManager == nullptr)
		return;

	eventManager->Consume<EvVfxSpawnRequest>([this](const EvVfxSpawnRequest& event)
	{
		// 지면 균열 데칼
		if (const std::optional<GroundDecalDesc> decal = ResolveGroundDecal(event.skillType, event.reason))
		{
			const Vec3 center = event.position + decal->positionOffset;
			DecalFactory::StampGroundCrack(mWorld, center, decal->radius,
				decal->texName ? decal->texName : L"", decal->color, decal->lifetime,
				decal->atlasCols, decal->atlasRows, decal->atlasIndex);
		}

		// event.rotation(오일러 degree) → 로컬 +Z 방향(총알 진행 방향 / 캐릭터 전방).
		const Vec3 eventForward = MathUtils::ForwardFromEulerDegrees(event.rotation);

		// 벽 총알 흔적 데칼 (투사체 벽 충돌 = CollisionStatic).
		if (const std::optional<BulletHoleDesc> hole = ResolveBulletHole(event.skillType, event.reason))
		{
			DecalFactory::StampBulletHole(mWorld, event.position, eventForward,
				hole->texName ? hole->texName : L"", hole->size, hole->color, hole->lifetime,
				hole->atlasCols, hole->atlasRows, hole->atlasIndex);
		}

		// 근접 평타 전방 흔적 (Fire). event.position(≈발밑)을 무기 높이로 올리고,
		// aim forward(피치 포함)로 전방 슬래브를 뻗어 사거리 내 벽 표면에 투영.
		if (const std::optional<BulletHoleDesc> melee = ResolveMeleeWallMark(event.skillType, event.reason))
		{
			const Vec3 origin = event.position + Vec3::Up * kMeleeWeaponHeight;
			DecalFactory::StampForwardWallMark(mWorld, origin, eventForward,
				kMeleeReachBack, kMeleeReachForward, melee->size,
				melee->texName ? melee->texName : L"", melee->color, melee->lifetime,
				melee->atlasCols, melee->atlasRows, melee->atlasIndex);
		}

		const std::optional<VfxSpawnDesc> desc = ResolveVfxSpawn(event.skillType, event.reason);
		if (!desc || desc->effectName == nullptr)
			return;

		const Vec3 spawnPosition = event.position + desc->positionOffset + eventForward * desc->forwardOffset;
		const Entity vfxEntity = PlayOneShot(desc->effectName, spawnPosition, event.rotation, desc->scale);

		// 시전자 추종 VFX는 패킷의 casterNetId로 시전자 엔티티를 찾아 부착한다.
		// (NetIdMap 조회라 적/플레이어/보스 다수 무관하게 정확히 매핑됨)
		if (desc->followCaster && event.casterNetId != 0 && vfxEntity.IsValid())
		{
			const Entity target = mWorld->GetEntityByNetId(event.casterNetId);
			if (target.IsValid())
			{
				if (VfxComponent* component = mWorld->GetComponent<VfxComponent>(vfxEntity))
				{
					component->mFollowTarget = target;
					component->mFollowOffset = desc->positionOffset;	// 스폰 오프셋을 추종 중에도 유지
					component->mFollowRotation = desc->followCasterRotation;
					component->mFollowInitialRotation = event.rotation;
					component->mStopWhenCasterLeavesSpecial = desc->loopUntilSpecialEnds;
					component->mFollowForwardOffset = desc->forwardOffset;
					if (desc->loopUntilSpecialEnds)
					{
						component->mIsLoop = true;
						component->mRestartWhenFinished = true;
					}
					if (event.skillType == SkillType::BaseUltimate)
					{
						component->mDebugCylinderLength = kBaseUltimateCollisionLength;
						component->mDebugCylinderRadius = kBaseUltimateCollisionRadius;
					}
				}
			}
		}
	});

	eventManager->Consume<EvAttachBulletVfx>([this](const EvAttachBulletVfx& event)
	{
		AttachBulletVfx(event.bullet, event.skillType, event.generation);
	});
}

void VfxSystem::ReturnFinishedEffects()
{
	for (Entity entity : mPool)
	{
		VfxComponent* component = mWorld->GetComponent<VfxComponent>(entity);
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		if (component == nullptr || transform == nullptr)
			continue;

		if (!component->mFinished)
			continue;

		// EffectPass가 종료를 감지한 원샷 VFX를 풀 대기 상태로 돌린다.
		component->ResetForPoolIdle();
		component->mIsPooled = true;
		component->mAutoReturn = true;
		SetHiddenTransform(*transform);
	}
}

void VfxSystem::UpdateFollowTargets()
{
	// 추종 대상이 지정된 풀 VFX의 transform을 매 프레임 대상 위치로 갱신한다.
	// (EffectPass가 Render phase에서 tr->mWorldPosition을 읽어 SetLocation 하므로 Post phase인 여기서 갱신)
	for (Entity entity : mPool)
	{
		VfxComponent* component = mWorld->GetComponent<VfxComponent>(entity);
		if (component == nullptr || !component->mInUse || component->mFinished)
			continue;
		if (!component->mShouldPlay && component->efkHandle == -1)
		{
			component->mFinished = true;
			continue;
		}
		if (!component->mFollowTarget.IsValid())
			continue;

		TransformComponent* targetTransform = mWorld->GetComponent<TransformComponent>(component->mFollowTarget);
		if (targetTransform == nullptr)
		{
			// 대상이 사라지면 추종을 멈추고 마지막 위치에 그대로 둔다.
			component->mFollowTarget = Entity{};
			continue;
		}

		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(entity);
		if (transform == nullptr)
			continue;

		if (component->mStopWhenCasterLeavesSpecial)
		{
			const MainPlayerComponent* caster =
				mWorld->GetComponent<MainPlayerComponent>(component->mFollowTarget);
			if (caster == nullptr ||
				caster->mUpperState != static_cast<int>(ReplicatedActionState::Special))
			{
				component->mRestartWhenFinished = false;
				component->mIsLoop = false;
				component->mShouldPlay = false;
				component->mStopWhenCasterLeavesSpecial = false;
				continue;
			}
		}

		Vec3 followRotation = component->mFollowInitialRotation;
		if (component->mFollowRotation)
		{
			// 로컬 플레이어는 카메라 조준을 그대로 사용한다. 원격 플레이어는
			// 동기화된 몸 yaw를 따라가되 스폰 패킷의 pitch를 유지한다.
			PlayerMovementComponent* movement =
				mWorld->GetComponent<LocalPlayerComponent>(component->mFollowTarget)
				? mWorld->GetComponent<PlayerMovementComponent>(component->mFollowTarget)
				: nullptr;
			if (movement != nullptr)
			{
				followRotation.x = movement->mCameraRotationX;
				followRotation.y = movement->mCameraRotationY;
			}
			else
			{
				followRotation.y = targetTransform->mLocalRotationE.y;
			}
		}

		const Vec3 followDirection = MathUtils::ForwardFromEulerDegrees(followRotation);
		const Vec3 followPosition = targetTransform->mWorldPosition + component->mFollowOffset +
			followDirection * component->mFollowForwardOffset;
		if (component->mFollowRotation)
		{
			transform->mLocalRotationE = followRotation;
			transform->mWorldMatrix =
				Matrix::CreateFromQuaternion(MathUtils::QuatFromEulerDegrees(followRotation)) *
				Matrix::CreateTranslation(followPosition);
		}
		else
		{
			transform->mWorldMatrix = Matrix::CreateTranslation(followPosition);
		}
		transform->mLocalPosition = followPosition;
		transform->mWorldPosition = followPosition;

		if (RenderSystem::GetDrawPlayerAttackRanges() &&
			component->mDebugCylinderLength > 0.0f && component->mDebugCylinderRadius > 0.0f)
		{
			const Vec3 direction = MathUtils::ForwardFromEulerDegrees(transform->mLocalRotationE);
			const Vec3 collisionStart = followPosition - direction * component->mFollowForwardOffset;
			SubmitDebugOrientedCylinder(
				collisionStart,
				direction,
				component->mDebugCylinderLength,
				component->mDebugCylinderRadius,
				Vec4(1.0f, 1.0f, 0.0f, 1.0f));
		}
	}
}

void VfxSystem::AttachBulletVfx(Entity bulletEntity, SkillType skillType, uint16 generation)
{
	if (!bulletEntity.IsValid())
		return;

	BulletComponent* bullet = mWorld->GetComponent<BulletComponent>(bulletEntity);
	if (bullet == nullptr || !bullet->mIsActive || bullet->mGeneration != generation)
		return;

	const BulletVfxDesc desc = ResolveBulletVfx(skillType);
	if (desc.effectName == nullptr)
		return;

	shared_ptr<Vfx> vfx = RESOURCEMANAGER.Get<Vfx>(desc.effectName);
	if (vfx == nullptr)
	{
#ifdef _DEBUG
		std::wcout << L"[VfxSystem] missing bullet vfx resource: " << desc.effectName << std::endl;
#endif
		return;
	}

	VfxComponent* bulletVfx = mWorld->GetComponent<VfxComponent>(bulletEntity);
	if (bulletVfx == nullptr)
		bulletVfx = &mWorld->AddComponent<VfxComponent>(bulletEntity);
	if (bulletVfx == nullptr)
		return;

	bulletVfx->mVfx = vfx;
	// VFX를 잘못 갱신 대비 끊기
	bulletVfx->efkHandle = -1;
	bulletVfx->mScale = desc.scale;
	bulletVfx->mIsLoop = true;

	bulletVfx->mRestartWhenFinished = true;
	bulletVfx->mIsPaused = false;
	bulletVfx->mIsPlaying = false;
	bulletVfx->mShouldPlay = true;
	// 폴링 끄기.
	bulletVfx->mIsPooled = false;
	bulletVfx->mInUse = false;
	bulletVfx->mAutoReturn = false;
	bulletVfx->mFinished = false;
	bulletVfx->mTotalTime = 0.f;
}

Entity VfxSystem::AcquirePooledEntity()
{
	for (Entity entity : mPool)
	{
		VfxComponent* component = mWorld->GetComponent<VfxComponent>(entity);
		if (component == nullptr)
			continue;

		if (component->mIsPooled && !component->mInUse)
			return entity;
	}

	return Entity{};
}

void VfxSystem::SetHiddenTransform(TransformComponent& transform)
{
	const Vec3 hiddenPosition(0.f, -100000.f, 0.f);
	transform.mLocalPosition = hiddenPosition;
	transform.mWorldPosition = hiddenPosition;
	transform.mLocalRotationE = Vec3::Zero;
	transform.mWorldMatrix = Matrix::CreateTranslation(hiddenPosition);
}


bool VfxSystem::IsBaseSkill(SkillType type)
{
	return type == SkillType::BaseAttack ||
		type == SkillType::BaseAttack2 ||
		type == SkillType::BaseAttack3 ||
		type == SkillType::BaseSkill1 ||
		type == SkillType::BaseSkill2;
}

bool VfxSystem::IsRangedBulletSkill(SkillType type)
{
	switch (type)
	{
		case SkillType::BaseAttack:
		case SkillType::BaseAttack2:
		case SkillType::BaseAttack3:
		case SkillType::BaseSkill1:
		case SkillType::GuitarAttack_1:
	case SkillType::GuitarAttack_2:
	case SkillType::GuitarAttack_3:
	case SkillType::HornAttack:
		return true;
	default:
		return false;
	}
}

bool VfxSystem::IsStaticImpactSkill(SkillType type)
{
	return IsBaseSkill(type) ||
		type == SkillType::GuitarAttack_1 ||
		type == SkillType::GuitarAttack_2 ||
		type == SkillType::GuitarAttack_3 ||
		type == SkillType::HornAttack;
}

std::optional<GroundDecalDesc> VfxSystem::ResolveGroundDecal(SkillType skillType, uint8 reason)
{
	switch (static_cast<EffectSpawnReason>(reason))
	{
	case EffectSpawnReason::Fire:
		// GuitarSkill1 = 지면 강타 스킬.
		if (skillType == SkillType::GuitarSkill1)
		{
			GroundDecalDesc d;
			d.texName    = L"UI_VFXDecal_Sheet";
			d.atlasCols  = kVfxDecalCols;
			d.atlasRows  = kVfxDecalRows;
			d.atlasIndex = kCellGroundCrack;
			d.radius     = 300.0f;
			d.color      = Vec4(1.0f, 1.0f, 1.0f, 1.0f);   // 원화 그대로(>1 이면 Bloom 발광)
			d.lifetime   = 1.5f;
			return d;
		}
		return std::nullopt;

	default:
		return std::nullopt;
	}
}

std::optional<BulletHoleDesc> VfxSystem::ResolveBulletHole(SkillType skillType, uint8 reason)
{
	// 이벤트 : 벽에 맞은 발사체
	if (static_cast<EffectSpawnReason>(reason) != EffectSpawnReason::CollisionStatic)
		return std::nullopt;

	BulletHoleDesc d;
	d.texName    = L"UI_VFXDecal_Sheet";
	d.atlasCols  = kVfxDecalCols;
	d.atlasRows  = kVfxDecalRows;
	d.atlasIndex = PickBulletHoleCell(skillType);  // 기타=2, 드럼=3, 그 외=1
	d.size       = 40.0f;
	d.color      = Vec4(1.0f, 1.0f, 1.0f, 1.0f);   // 원화 그대로
	d.lifetime   = 12.0f;
	return d;
}

std::optional<BulletHoleDesc> VfxSystem::ResolveMeleeWallMark(SkillType skillType, uint8 reason)
{
	// 근접 평타 발동(Fire) 시, 전방 사거리 내 벽에 흔적.
	if (static_cast<EffectSpawnReason>(reason) != EffectSpawnReason::Fire)
		return std::nullopt;

	if (!IsMeleeWallMarkAttack(skillType))
		return std::nullopt;

	BulletHoleDesc d;
	d.texName    = L"UI_VFXDecal_Sheet";
	d.atlasCols  = kVfxDecalCols;
	d.atlasRows  = kVfxDecalRows;
	d.atlasIndex = PickBulletHoleCell(skillType);  // 기타=2, 드럼=3, 그 외=1
	d.size       = kMeleeMarkSize;
	d.color      = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	d.lifetime   = 12.0f;
	return d;
}

std::optional<VfxSpawnDesc> VfxSystem::ResolveVfxSpawn(SkillType skillType, uint8 reason)
{
	switch (static_cast<EffectSpawnReason>(reason))
	{
	case EffectSpawnReason::Fire:
		if (skillType == SkillType::GuitarUltimate)
			return VfxSpawnDesc{ L"VFX_Fanthor_Ultimate", Vec3::Zero, Vec3(10.0f) };

		if (skillType == SkillType::BaseUltimate)
			return VfxSpawnDesc{ L"VFX_Ibanix_Ultimate", Vec3(0.f, 110.f, 0.f), Vec3(1.0f),
				/*followCaster*/ true, /*followCasterRotation*/ true, /*loopUntilSpecialEnds*/ true,
				/*forwardOffset*/ 0.0f };

		if (IsRangedBulletSkill(skillType))
			return std::nullopt;

		/*if (skillType == SkillType::GuitarAttack)
			return VfxSpawnDesc{ L"VFX_Fanthor_Slash_01", Vec3(0.f, 100.f, 0.f), Vec3(30.0f) };*/

			if (skillType == SkillType::GuitarSkill1)
				return VfxSpawnDesc{ L"VFX_Fanthor_Skill_01", Vec3(0.f, 20.f, 0.f), Vec3(30.0f) };

		if (skillType == SkillType::DrumSkill1)
			return VfxSpawnDesc{ L"VFX_Rudwig_Skill_01", Vec3(0.f, 100.f, 80.f), Vec3(10.0f) };

		if (skillType == SkillType::DrumSkill2)
			return VfxSpawnDesc{ L"VFX_Rudwig_Skill_02", Vec3(0.f, 100.f, 80.f), Vec3(10.0f) };

		if (skillType == SkillType::DrumSkill3)
			return VfxSpawnDesc{ L"VFX_Rudwig_Reload", Vec3(0.f, 100.f, 80.f), Vec3(10.0f) };

		if (skillType == SkillType::PianoAttack)
			return VfxSpawnDesc{ L"VFX_Pianoman_Attack_01", Vec3(0.f, 100.f, 0.f), Vec3(15.0f) };

		if (skillType == SkillType::SlimeAttack)
			return VfxSpawnDesc{ L"VFX_Bongoman_Attack_01", Vec3(0.f, 20.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::FlyAttack)
			return VfxSpawnDesc{ L"VFX_Bongoman_Attack_01", Vec3(0.f, 20.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::BongoAttack)
			return VfxSpawnDesc{ L"VFX_Bongoman_Attack_01", Vec3(0.f, 20.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::BongoShild)
			return VfxSpawnDesc{ L"VFX_Bongoman_Shield", Vec3(0.f, 100.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::DragonSkill1 ||
			skillType == SkillType::DragonSkill3 ||
			skillType == SkillType::DragonSkill4)
			return VfxSpawnDesc{ L"VFX_Bongoman_Attack_01", Vec3(0.f, 20.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::DragonSkill2)
			return VfxSpawnDesc{ L"VFX_Bongoman_Shield", Vec3(0.f, 100.f, 0.f), Vec3(100.0f) };

		if (skillType == SkillType::BrassSkill1)
			return VfxSpawnDesc{ L"VFX_BrassBoss_Skill_01", Vec3(0.f, 100.f, 0.f), Vec3(1.0f), /*followCaster*/ true };

		if (skillType == SkillType::BrassSkill3)
			return VfxSpawnDesc{ L"VFX_BrassBoss_Skill_03", Vec3(0.f, 100.f, 0.f), Vec3(30.0f), /*followCaster*/ true };

		return std::nullopt;

	case EffectSpawnReason::CollisionEntity:
		if (IsBaseSkill(skillType))
			return VfxSpawnDesc{ L"VFX_Ibanix_Attack_Hit_01", Vec3::Zero, Vec3(30.0f) };

			if (skillType == SkillType::DrumAttack ||
				skillType == SkillType::DrumAttack3 ||
				skillType == SkillType::DrumSkill1)
				return VfxSpawnDesc{ L"VFX_Rudwig_Attack_Hit", Vec3(0.f, 100.f, 0.f), Vec3(20.0f) };

		if (skillType == SkillType::GuitarAttack ||
			skillType == SkillType::GuitarSkill1)
			return VfxSpawnDesc{ L"VFX_Fanthor_Attack_Hit", Vec3(0.f, 100.f, 0.f), Vec3(20.0f) };

		if (skillType == SkillType::GuitarAttack_1 ||
			skillType == SkillType::GuitarAttack_2 ||
			skillType == SkillType::GuitarAttack_3)
			return VfxSpawnDesc{ L"VFX_Fanthor_Attack_Hit", Vec3::Zero, Vec3(20.0f) };

		return std::nullopt;

	case EffectSpawnReason::CollisionStatic:
		if (IsStaticImpactSkill(skillType))
			return VfxSpawnDesc{ L"VFX_Ibanix_Attack_Hit_01", Vec3::Zero, Vec3(30.0f) };

		return std::nullopt;

	case EffectSpawnReason::Respawn:
		return VfxSpawnDesc{ L"VFX_Player_Rebirth", Vec3::Zero, Vec3(100.0f) };

	case EffectSpawnReason::CheckpointReached:
		return VfxSpawnDesc{ L"VFX_Monster_Spawn", Vec3(0.f, 100.f, 0.f), Vec3(5.0f) };

	case EffectSpawnReason::Death:
		if (skillType == SkillType::BrassSkill1)
			return VfxSpawnDesc{ L"VFX_BrassBoss_Die", Vec3(0.f, 100.f, 0.f), Vec3(100.0f) };
		return std::nullopt;

	default:
		return std::nullopt;
	}
}

BulletVfxDesc VfxSystem::ResolveBulletVfx(SkillType skillType)
{
	switch (skillType)
	{
	/*case SkillType::GuitarAttack:
		return BulletVfxDesc{ L"VFX_Fanthor_Slash_01", Vec3(12.0f, 12.0f, 12.0f) };*/
	case SkillType::GuitarAttack_1:
		return BulletVfxDesc{ L"VFX_Fanthor_Bullet_Slash_1", Vec3(1.0f, 1.0f, 1.0f) };
	case SkillType::GuitarAttack_2:
		return BulletVfxDesc{ L"VFX_Fanthor_Bullet_Slash_2", Vec3(1.0f, 1.0f, 1.0f) };
	case SkillType::GuitarAttack_3:
		return BulletVfxDesc{ L"VFX_Fanthor_Bullet_Slash_3", Vec3(1.0f, 1.0f, 1.0f) };

		case SkillType::BaseAttack:
		case SkillType::BaseAttack2:
		case SkillType::BaseAttack3:
		case SkillType::BaseSkill1:
			return BulletVfxDesc{ L"VFX_Ibanix_Bullet", Vec3(12.0f, 12.0f, 12.0f) };

		case SkillType::HornAttack:
			return BulletVfxDesc{ L"VFX_Hornman_Bullet", Vec3(12.0f, 12.0f, 12.0f) };

		case SkillType::BrassSkill2:
			return BulletVfxDesc{ L"VFX_BrassBoss_Skill_02", Vec3(150.0f, 150.0f, 150.0f) };
		case SkillType::BrassSkill3:
			return BulletVfxDesc{ L"VFX_Hornman_Bullet", Vec3(10.0f, 10.0f, 10.0f) };
		case SkillType::BrassSkill4:
			return BulletVfxDesc{ L"VFX_BrassBoss_Skill_02", Vec3(60.0f, 60.0f, 60.0f) };

		default:
			return BulletVfxDesc{ L"VFX_Ibanix_Bullet", Vec3(2.0f, 2.0f, 2.0f) };
		}
	}
