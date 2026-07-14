#pragma once
#include "UIFeature.h"
#include "Entity.h"

class UIEmoteFeature : public UIFeature
{
public:
	void Initialize(World* world) override;
	void Update(float dt) override;

private:
	void EnsureWheelEntities();
	void ConsumeEmoteEvents();
	void UpdateWheel();
	void UpdateDisplays(float dt);
	void ShowEmote(Entity caster, uint8 emoteId);
	Entity CreateDisplayEntity(Entity caster, uint8 emoteId);
	void ApplyEmoteSourceRect(Entity entity, uint8 emoteId);
	bool ProjectWorldToScreen(const Vec3& worldPosition, Vec2& outScreenPosition) const;

private:
	static constexpr size_t WheelEntityCount = 10;
	std::array<Entity, WheelEntityCount> mWheelEntities{};
};
