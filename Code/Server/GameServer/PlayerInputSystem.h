#pragma once
#include "World.h"
#include "System.h"
#include "BulletComponent.h"
#include "Protocol/RhythmDefinitions.h"

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
	bool EnqueueAttackEventByCategory(
		EventManager& eventManager,
		Entity shooter,
		SkillType bulletType,
		bool isCritical,
		bool isOnBeat);

	SkillType ResolveSkillType(
		uint8 playerType,
		InputButtons actionButton,
		Rhythm rhythm = Rhythm::Neutral);
	uint8 EvaluateBeatJudgement(const MainPlayerComponent* mp, const InputComponent* inputComp, const BeatSystem* beatSystem) const;

	void EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo);
	void StartBaseUltimate(Entity player, MainPlayerComponent* playerComponent,
	                       InputComponent* input, EventManager& eventManager,
	                       float now, float beatSeconds);
	bool TickBaseUltimate(Entity player, MainPlayerComponent* playerComponent,
	                      InputComponent* input, EventManager& eventManager,
	                      float now, float beatSeconds, float dt, bool cancelRequested);
	void ApplyBaseUltimateDamage(Entity player, const InputComponent& input,
	                             EventManager& eventManager);
	void StartGuitarUltimate(MainPlayerComponent* playerComponent, float now, float beatSeconds);
	bool TickGuitarUltimate(Entity player, MainPlayerComponent* playerComponent,
	                        InputComponent* input, EventManager& eventManager,
	                        float now, float beatSeconds, float dt);
	void ApplyGuitarUltimateDamage(Entity player, const Vec3& center, EventManager& eventManager);
	void StartDrumUltimate(Entity player, MainPlayerComponent* playerComponent,
	                       EventManager& eventManager, float now, float beatSeconds);
	bool TickDrumUltimate(Entity player, MainPlayerComponent* playerComponent,
	                     InputComponent* input, EventManager& eventManager,
	                     float now, float dt);
	void ApplyDrumUltimateExplosion(Entity player, int32 hitCount, EventManager& eventManager);

	bool TryFireAction(Entity e, MainPlayerComponent* mp, EventManager& em,
	                   InputButtons button, float now, float Beat,
	                   bool isCritical = false, bool isOnBeat = false);

	// 입력 순간 곡 위치로 박자 판정
	void JudgeAndNotify(Entity e, MainPlayerComponent* mp, InputComponent* inputComp,
	                    BeatSystem* beatSystem, InputButtons button);

	// 리듬 콤보
	static bool IsComboAttackButton(InputButtons button);
	// 판정 결과 반영
	void ApplyComboJudgement(Entity e, MainPlayerComponent* mp, InputButtons button, uint8 judgement);
	// 적 피격 확정(EvHitConfirm) 소비
	void ConsumeComboHitConfirms();
	// 무공격 3초 경과 시 콤보 만료 처리.
	void TickComboTimeout(Entity e, MainPlayerComponent* mp, float now);
	// 콤보 변경을 해당 플레이어에게 unicast.
	void SendComboChanged(Entity e, int32 combo, uint8 reason);

public:
	const float mDPI = 5.f;
};
