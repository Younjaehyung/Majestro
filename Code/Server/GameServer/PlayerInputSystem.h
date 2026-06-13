#pragma once
#include "World.h"
#include "System.h"
#include "BulletComponent.h"

class MainPlayerComponent;
class InputComponent;
class BeatSystem;
enum class InputButtons : uint8;

class PlayerInputSystem : public System
{
public:
	PlayerInputSystem(World* world);

	void Initialize();
	void Update(float dt);

private:
	bool EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType);

	SkillType ResolveSkillType(uint8 playerType, InputButtons actionButton, uint8 rhythm = 0);

	void EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo);

	bool TryFireAction(Entity e, MainPlayerComponent* mp, EventManager& em,
	                   InputButtons button, float now, float Beat);

	// 입력 순간 곡 위치로 박자 판정
	void JudgeAndNotify(Entity e, MainPlayerComponent* mp, InputComponent* inputComp,
	                    BeatSystem* beatSystem, InputButtons button);

public:
	const float mDPI = 5.f;
};