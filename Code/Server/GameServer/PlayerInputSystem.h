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
	bool EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType, bool isCritical);

	SkillType ResolveSkillType(uint8 playerType, InputButtons actionButton, uint8 rhythm = 0);
	uint8 EvaluateBeatJudgement(const MainPlayerComponent* mp, const InputComponent* inputComp, const BeatSystem* beatSystem) const;

	void EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo);

	bool TryFireAction(Entity e, MainPlayerComponent* mp, EventManager& em,
	                   InputButtons button, float now, float Beat, bool isCritical = false);

	// 입력 순간 곡 위치로 박자 판정
	void JudgeAndNotify(Entity e, MainPlayerComponent* mp, InputComponent* inputComp,
	                    BeatSystem* beatSystem, InputButtons button);

	// 리듬 콤보
	// 콤보를 쌓는 '공격' 버튼인지 판정 (ATTACK/SKILL1/SKILL2).
	static bool IsComboAttackButton(InputButtons button);
	// 판정 결과로 콤보 갱신: Perfect/Good : +1, Miss : 0. 변화 시 클라에 통지.
	void ApplyComboJudgement(Entity e, MainPlayerComponent* mp, InputButtons button, uint8 judgement);
	// 무공격 3초 경과 시 콤보 만료 처리.
	void TickComboTimeout(Entity e, MainPlayerComponent* mp, float now);
	// 콤보 변경을 해당 플레이어에게 unicast.
	void SendComboChanged(Entity e, int32 combo, uint8 reason);

public:
	const float mDPI = 5.f;
};
