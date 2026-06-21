#pragma once
#include "System.h"
#include "AnimNotifyComponent.h"

class TransformSystem;
class AnimationComponent;
class VfxSystem;

class AnimNotifySystem : public System
{
public:
	AnimNotifySystem(World* world);

	void Initialize() override;
	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;

private:
	void LoadTable(const std::string& path);

	void ProcessLayer(Entity owner, AnimationComponent& anim, bool useUpper,
		AnimNotifyComponent::LayerTrack& track);

	void Fire(Entity owner, const AnimNotifyEntry& entry, float frameDuration);
	void FireCameraShake(Entity owner, const AnimNotifyEntry& entry, float frameDuration);
	void FireCameraDolly(Entity owner, const AnimNotifyEntry& entry, float frameDuration);
	bool ResolveAnchor(Entity owner, const AnimNotifyEntry& entry, Vec3& outPos) const;

	void UpdateActiveFollows();

	VfxSystem* GetVfxSystem();

private:
	// 클립명(Object::GetName(), wstring) -> 엔트리 목록
	std::unordered_map<std::wstring, std::vector<AnimNotifyEntry>> mTable;

	// follow=true VFX가 소켓을 수명 동안 추적하기 위한 활성 목록
	struct ActiveFollow
	{
		Entity      vfx;          // PlayOneShot이 반환한 풀 엔티티
		Entity      src;          // 소켓 보유 엔티티(플레이어)
		std::string socket;
		Vec3        offset;
	};
	std::vector<ActiveFollow> mActiveFollows;

	VfxSystem* mVfxSystem = nullptr; 
};
