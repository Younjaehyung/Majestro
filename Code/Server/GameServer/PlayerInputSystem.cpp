#include "pch.h"
#include "PlayerInputSystem.h"

#include "PlayerSystem.h"
#include "PlayerComponent.h"
#include "GravityComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "NetEntityComponent.h"
#include "ServerCore.h"
#include "BulletComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "BeatSystem.h"
#include "GameTimer.h"
#include "BuffComponent.h"
#include "RhythmComponents.h"
#include "EnemyComponent.h"


PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;
	auto eventManager = mWorld->GetEventManager();
	if (not eventManager) return;


	ConsumeComboHitConfirms();

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	for (auto& e : entitys) {
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(e);
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(e);
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
		BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(e);

		// 사망
		if (mainPlayerComponent && mainPlayerComponent->IsDeathActive())
		{
			mainPlayerComponent->mSpeed = 0.f;
			mainPlayerComponent->mHasMoveInput = false;
			continue;
		}

		auto systemManager = mWorld->GetSystemManager();
		auto* beatSystem = systemManager->GetSystem<BeatSystem>();
		const float Beat = beatSystem->mBpmSeconds;
		const float now  = GetServerTotalTimeSeconds();

		// 리듬 콤보: 무공격 타임아웃 검사(서버 권위)
		TickComboTimeout(e, mainPlayerComponent, now);

		constexpr float MoveInputTimeout = 0.25f;
		if (inputComp->mLastMoveInputTime >= 0.0f &&
			now - inputComp->mLastMoveInputTime > MoveInputTimeout)
		{
			// 씬 로딩이나 네트워크 단절로 키 해제 패킷을 받지 못해도 이동 입력이 고착되지 않게 한다
			inputComp->MoveX = 0.0f;
			inputComp->MoveY = 0.0f;
			inputComp->MoveZ = 0.0f;
		}

		//연속행동
		if (mainPlayerComponent->mPendingAction != PendingAction::None)
		{
			InputButtons btn = InputButtons::ATTACK;
			switch (mainPlayerComponent->mPendingAction)
			{
			case PendingAction::Attack: btn = InputButtons::ATTACK; break;
			case PendingAction::Skill1: btn = InputButtons::SKILL1; break;
			case PendingAction::Skill2: btn = InputButtons::SKILL2; break;
			case PendingAction::Reload: btn = InputButtons::RELOAD; break;
			default: break;
			}
			TryFireAction(e, mainPlayerComponent, *eventManager, btn, now, Beat);
			mainPlayerComponent->mPendingAction = PendingAction::None; // 소비 완료
		}


		mainPlayerComponent->mHasMoveInput =
			std::abs(inputComp->MoveX) > 0.01f ||
			std::abs(inputComp->MoveZ) > 0.01f;

		const bool dance1Active = (mainPlayerComponent->mFsm.GetState() == S_Dance1);
		const bool hasActionState =
			mainPlayerComponent->GetReplicatedActionState() != static_cast<uint8>(ReplicatedActionState::None);
		const bool canUseHorizontalInput = mainPlayerComponent->CanUseHorizontalInput();

		if (!mainPlayerComponent->mHasMoveInput) {
			mainPlayerComponent->mSpeed = 0.f;
			if (!hasActionState)
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
		}
		else {
			if (mainPlayerComponent->mDash) mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
			else if (canUseHorizontalInput)
			{
				float variantMoveMultiplier = 1.0f;
				if (const RhythmEffectComponent* rhythmEffect =
					mWorld->GetComponent<RhythmEffectComponent>(e))
				{
					variantMoveMultiplier = rhythmEffect->GetVariantModifiers().moveSpeedMultiplier;
				}
				mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed *
					buffComp->mMoveSpeedMultiplier * variantMoveMultiplier;
			}
			else mainPlayerComponent->mSpeed = 0.f;
		}
		

		//movementComponent->mMovingDirection = { 0,0,0 };
		mainPlayerComponent->mPlayerMovingDir.x = inputComp->MoveZ;
		mainPlayerComponent->mPlayerMovingDir.y = inputComp->MoveX;
		if (mainPlayerComponent->mDash) {
			inputComp->MoveZ = 1;
			inputComp->MoveX = 0;
		}
		else if (canUseHorizontalInput && (!hasActionState || dance1Active)) {
			if (inputComp->MoveX == 1) {
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunRightState::Instance());
			}
			if (inputComp->MoveX == -1) {
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunLeftState::Instance());
			}
			if (inputComp->MoveZ == 1) {
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunForwardState::Instance());
			}
			if (inputComp->MoveZ == -1) {
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunBackwardState::Instance());
			}
		}


		if (inputComp->IsButtonPressed(InputButtons::SPACE)) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SHIFT)) {
			//cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}

		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {
			const uint8 judgement = EvaluateBeatJudgement(mainPlayerComponent, inputComp, beatSystem);
			if (TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::ATTACK, now, Beat,
				judgement == static_cast<uint8>(BeatJudgement::Perfect)))
			{
				JudgeAndNotify(e, mainPlayerComponent, inputComp, beatSystem, InputButtons::ATTACK);
			}
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			const uint8 judgement = EvaluateBeatJudgement(mainPlayerComponent, inputComp, beatSystem);
			if (TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::SKILL1, now, Beat,
				judgement == static_cast<uint8>(BeatJudgement::Perfect)))
				JudgeAndNotify(e, mainPlayerComponent, inputComp, beatSystem, InputButtons::SKILL1);
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			const uint8 judgement = EvaluateBeatJudgement(mainPlayerComponent, inputComp, beatSystem);
			if (TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::SKILL2, now, Beat,
				judgement == static_cast<uint8>(BeatJudgement::Perfect)))
				JudgeAndNotify(e, mainPlayerComponent, inputComp, beatSystem, InputButtons::SKILL2);
		}

		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {
			const uint8 judgement = EvaluateBeatJudgement(mainPlayerComponent, inputComp, beatSystem);
			if (TryFireAction(e, mainPlayerComponent, *eventManager, InputButtons::RELOAD, now, Beat,
				judgement == static_cast<uint8>(BeatJudgement::Perfect)))
				JudgeAndNotify(e, mainPlayerComponent, inputComp, beatSystem, InputButtons::RELOAD);
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RhythmChangeState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::DANCE1)) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Dance1State::Instance());
		}

		//cout << "bullet: " << mainPlayerComponent->mNowBullet << endl;
		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}

uint8 PlayerInputSystem::EvaluateBeatJudgement(const MainPlayerComponent* mp, const InputComponent* inputComp, const BeatSystem* beatSystem) const
{
	(void)mp;
	if (mp == nullptr || inputComp == nullptr || beatSystem == nullptr)
		return static_cast<uint8>(BeatJudgement::Miss);

	const float beatSec = beatSystem->mBpmSeconds;
	const float serverSongPos = beatSystem->GetSongPosition();
	const float inputSongPos = inputComp->InputSongPos;
	const float drift = serverSongPos - inputSongPos;
	if (drift < -kMaxInputSongPosDrift || drift > kMaxInputSongPosDrift)
		return static_cast<uint8>(BeatJudgement::Miss);

	return ClassifyBeatJudgement(inputSongPos, beatSec);
}


void PlayerInputSystem::JudgeAndNotify(Entity e, MainPlayerComponent* mp, InputComponent* inputComp,
                                      BeatSystem* beatSystem, InputButtons button)
{
	if (mp == nullptr || inputComp == nullptr || beatSystem == nullptr)
		return;

	const float inputSongPos  = inputComp->InputSongPos;

	// 같은 입력에 대한 중복 판정 방지
	if (inputSongPos == mp->mLastJudgedInputSongPos)
		return;
	mp->mLastJudgedInputSongPos = inputSongPos;

	// 판정은 입력 순간 곡 위치 기준. 
	const uint8 judgement = EvaluateBeatJudgement(mp, inputComp, beatSystem);

	mp->mLastBeatJudgement = judgement; // 판정 저장

	// 행동한 플레이어에게만 unicast 통지
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr || netComp->mSessionId == 0)
		return;

	// 리듬 콤보 갱신(서버 권위). 판정 결과에 따라 누적/초기화 후 변화 시 클라에 통지.
	ApplyComboJudgement(e, mp, button, judgement);

	S2C_BeatJudgementPacket pkt{};
	pkt.netEntityId  = netComp->mNetEntityId;
	pkt.judgement    = judgement;
	pkt.actionButton = static_cast<uint8>(button);

	SendRequest req{ netComp->mSessionId, PKT_Type::S2C_PKT_BEAT_JUDGEMENT, sizeof(S2C_BeatJudgementPacket) };
	req.StoreAs<S2C_BeatJudgementPacket>(pkt);
	gSendQueue.Push(req);

	cout << "[BeatJudge] session " << netComp->mSessionId
	     << " btn " << static_cast<int>(button) << " => " << static_cast<int>(judgement)
	     << " (inputPos " << inputSongPos << ")" << endl;
}

bool PlayerInputSystem::IsComboAttackButton(InputButtons button)
{
	// 일반/스킬 공격만 콤보 대상. RELOAD 는 제외. (SPECIAL 은 서버에서 리듬 변경이라 판정 자체가 없음)
	switch (button)
	{
	case InputButtons::ATTACK:
	case InputButtons::SKILL1:
	case InputButtons::SKILL2:
		return true;
	default:
		return false;
	}
}

void PlayerInputSystem::ApplyComboJudgement(Entity e, MainPlayerComponent* mp, InputButtons button, uint8 judgement)
{
	if (mp == nullptr || !IsComboAttackButton(button))
		return;

	const float now = GetServerTotalTimeSeconds();

	// 후속타 Miss 는 콤보 리셋 면제
	const uint32 state = mp->GetState();
	const bool isComboFollowup = (state == S_ComboAttack1 || state == S_ComboAttack2);

	if (judgement == static_cast<uint8>(BeatJudgement::Miss))
	{
		// 후속타 Miss 는 리셋 면제
		if (isComboFollowup)
			return;

		// 첫 타/스킬의 Miss시 콤보 끊김
		mp->mComboHitEligibleUntil = 0.0f;
		mp->mComboHitTargetCount = 0;
		if (mp->mRhythmCombo != 0)
		{
			mp->mRhythmCombo = 0;
			SendComboChanged(e, 0, /*reason=Miss*/ 1);
		}
	}
	else
	{
		mp->mComboHitEligibleUntil = now + MainPlayerComponent::kComboHitWindow;
		mp->mComboHitTargetCount = 0;
	}
}

void PlayerInputSystem::ConsumeComboHitConfirms()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	const float now = GetServerTotalTimeSeconds();

	eventManager->Consume<EvHitConfirm>([&](const EvHitConfirm& e)
	{
		if (!e.instigator.IsValid() || !e.target.IsValid())
			return;

		// 적 피격만 콤보 대상
		if (mWorld->GetComponent<EnemyComponent>(e.target) == nullptr)
			return;

		MainPlayerComponent* mp = mWorld->GetComponent<MainPlayerComponent>(e.instigator);
		if (mp == nullptr)
			return;

		// 박자 적중 공격의 피격 시간내에만 콤보 증가
		if (now > mp->mComboHitEligibleUntil)
			return;

		// 같은 적은 한 공격당 1회만 인정
		const uint32 targetId = e.target.GetID();
		for (uint8 index = 0; index < mp->mComboHitTargetCount; ++index)
		{
			if (mp->mComboHitTargets[index] == targetId)
				return;
		}
		if (mp->mComboHitTargetCount >= MainPlayerComponent::kComboHitTargetsMax)
			return; // 한 공격당 최대 8명까지 인정
		mp->mComboHitTargets[mp->mComboHitTargetCount++] = targetId;

		mp->mRhythmCombo += 1;
		mp->mRhythmComboExpireTime = now + MainPlayerComponent::kRhythmComboTimeout;
		SendComboChanged(e.instigator, mp->mRhythmCombo, /*reason=Increment*/ 0);

	});
}

void PlayerInputSystem::TickComboTimeout(Entity e, MainPlayerComponent* mp, float now)
{
	if (mp == nullptr || mp->mRhythmCombo <= 0)
		return;

	if (now > mp->mRhythmComboExpireTime)
	{
		mp->mRhythmCombo = 0;
		SendComboChanged(e, 0, /*reason=Timeout*/ 2);
	}
}

void PlayerInputSystem::SendComboChanged(Entity e, int32 combo, uint8 reason)
{
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	if (netComp == nullptr || netComp->mSessionId == 0)
		return;

	S2C_ComboChangedPacket pkt{};
	pkt.netEntityId = netComp->mNetEntityId;
	pkt.comboCount  = combo;
	pkt.reason      = reason;

	SendRequest req{ netComp->mSessionId, PKT_Type::S2C_PKT_COMBO_CHANGED, sizeof(S2C_ComboChangedPacket) };
	req.StoreAs<S2C_ComboChangedPacket>(pkt);
	gSendQueue.Push(req);
}

bool PlayerInputSystem::EnqueueAttackEventByCategory(EventManager& eventManager, Entity shooter, SkillType bulletType, bool isCritical)
{
	switch (bulletType)
	{
		case SkillType::DrumAttack:
		case SkillType::DrumAttack3:
		case SkillType::DrumSkill1:
		case SkillType::GuitarAttack:
		case SkillType::GuitarSkill1:
		eventManager.Enqueue<EvMeleeAttackRequest>({ shooter, bulletType, isCritical });
		return true;

		case SkillType::BaseAttack:
		case SkillType::BaseAttack2:
		case SkillType::BaseAttack3:
		case SkillType::BaseSkill1:
		case SkillType::GuitarAttack_1:
		case SkillType::GuitarAttack_2:
	case SkillType::GuitarAttack_3:
		eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType, isCritical });
		return true;


	case SkillType::DrumSkill2:
	case SkillType::DrumSkill3:
		eventManager.Enqueue<EvBuffRequest>({ shooter, bulletType });
		// 버프 시전을 지원 행동으로 발행 → ScoreSystem이 어시스트 창 판정에 사용한다.
		eventManager.Enqueue<EvSupportAction>(EvSupportAction{ shooter });
		return true;

	case SkillType::Default:
	default:
		//eventManager.Enqueue<EvRangedAttackRequest>({ shooter, bulletType });
		return true;
	}
}

SkillType PlayerInputSystem::ResolveSkillType(
	uint8 playerType,
	InputButtons actionButton,
	Rhythm rhythm)
{
	switch (playerType)
	{
	case PlayerType::Rudwig:
		switch (actionButton)
		{
		case InputButtons::ATTACK: return SkillType::DrumAttack;
		case InputButtons::SKILL1: return SkillType::DrumSkill1;
		case InputButtons::SKILL2: return SkillType::DrumSkill2;
		case InputButtons::RELOAD: return SkillType::DrumSkill3;
		default: return SkillType::Default;
		}
	case PlayerType::Ibanix:
		switch (actionButton)
		{
		case InputButtons::ATTACK: return SkillType::BaseAttack;
		case InputButtons::SKILL1: return SkillType::BaseSkill1;
		case InputButtons::SKILL2: return SkillType::BaseSkill2;
		case InputButtons::RELOAD: return SkillType::BaseSkill3;
		default: return SkillType::Default;
		}
	case PlayerType::Fanthor:
		switch (actionButton)
		{
		case InputButtons::ATTACK:
			switch (rhythm) {
			case Rhythm::R1: return SkillType::GuitarAttack_1;
			case Rhythm::R2: return SkillType::GuitarAttack_2;
			case Rhythm::R3: return SkillType::GuitarAttack_3;
			default: return SkillType::GuitarAttack;
			}

		case InputButtons::SKILL1: return SkillType::GuitarSkill1;
		case InputButtons::SKILL2: return SkillType::GuitarSkill2;
		case InputButtons::RELOAD: return SkillType::GuitarSkill3;
		default: return SkillType::Default;
		}
	default:
		return SkillType::Default;
	}
}

void PlayerInputSystem::EnqueueAmmoChangedIfNeeded(World* world, EventManager& eventManager, Entity playerEntity, int prevAmmo)
{
	MainPlayerComponent* playerComp = world->GetComponent<MainPlayerComponent>(playerEntity);
	if (!playerComp)
		return;

	if (playerComp->mNowBullet == prevAmmo)
		return;

	eventManager.Enqueue<EvAmmoChanged>({
		playerEntity,
		static_cast<int32>(playerComp->mNowBullet),
		static_cast<int32>(playerComp->mMaxBullet)
		});
}

bool PlayerInputSystem::TryFireAction(Entity e, MainPlayerComponent* mp, EventManager& em,
	                                  InputButtons button, float now, float Beat, bool isCritical)
{
	if (mp == nullptr) return false;
	if (GravityComponent* gravityComp = mWorld->GetComponent<GravityComponent>(e))
	{
		if (gravityComp->mFalling && button != InputButtons::RELOAD)
			return false;
	}

	float* nextTimePtr = nullptr;
	float  cool = 0.f;
	uint8  cdSlot = 0xFF;       // 0=Attack,1=Skill1,2=Skill2,3=Reload (쿨타임 UI 슬롯)
	State<MainPlayerComponent>* nextState = nullptr;
	bool   needsAmmo = false;

	switch (button)
	{
	case InputButtons::ATTACK:
		nextTimePtr = &mp->mNextAttackTime;
		cool        = mp->mAttackCool;
		cdSlot      = 0;
		nextState   = Attack1State::Instance();
		needsAmmo   = (mp->mPlayerType == Ibanix); // Fanthor 탄0 기본공격 유지
		break;
	case InputButtons::SKILL1:
		nextTimePtr = &mp->mNextSkill1Time;
		cool        = mp->mSkill1Cool;
		cdSlot      = 1;
		nextState   = Skill1State::Instance();
		needsAmmo   = (mp->mPlayerType == Ibanix); // 기존 룰
		break;
	case InputButtons::SKILL2:
		nextTimePtr = &mp->mNextSkill2Time;
		cool        = mp->mSkill2Cool;
		cdSlot      = 2;
		nextState   = Skill2State::Instance();
		needsAmmo   = false;
		break;
	case InputButtons::RELOAD:
		nextTimePtr = &mp->mNextReloadTime;
		cool        = mp->mReloadCool;
		cdSlot      = 3;
		nextState   = ReloadState::Instance();
		needsAmmo   = false;
		break;
	default:
		return false;
	}

	if (nextTimePtr == nullptr || nextState == nullptr) return false;
	if (*nextTimePtr > now) return false;
	if (needsAmmo && mp->mNowBullet <= 0) return false;

	if (button == InputButtons::ATTACK)
	{
		if (now > mp->mComboExpireTime)
			mp->mComboStep = 0;

		switch (mp->mComboStep)
		{
		case 1:
			nextState = ComboAttack1State::Instance();
			break;
		case 2:
			nextState = ComboAttack2State::Instance();
			break;
		default:
			nextState = Attack1State::Instance();
			break;
		}
	}

	const bool delaySkill1Attack =
		button == InputButtons::SKILL1 &&
		(mp->mPlayerType == Rudwig || mp->mPlayerType == Fanthor);

	if (delaySkill1Attack)
	{
		if (mp->mStateThrew)
		{
			mp->mStateThrew = false;
			mp->mFsm.ChangeState(mp, nextState);
			return true;
		}

		if (mp->GetState() == S_Skill1 && mp->mPendingAction != PendingAction::Skill1)
			return false;
	}

		// Fanthor 기본공격:
		// silence 상태면 항상 근접 GuitarAttack만 사용한다.
		// 그 외에는 rhythm 0 은 근접, rhythm 1~3 은 탄이 있으면 해당 원거리 공격을 사용한다.
		Rhythm rhythm = Rhythm::Neutral;
		if (button == InputButtons::ATTACK && mp->mPlayerType == Fanthor)
		{
			const BuffComponent* buffComp = mWorld->GetComponent<BuffComponent>(e);
			const bool isSilenced = buffComp && buffComp->FindBuff(BuffType::Silence) != nullptr;
			const RhythmStateComponent* rhythmState =
				mWorld->GetComponent<RhythmStateComponent>(e);
			rhythm = isSilenced || !rhythmState
				? Rhythm::Neutral
				: rhythmState->GetCurrentRhythm();
			if (rhythm != Rhythm::Neutral && mp->mNowBullet <= 0)
				rhythm = Rhythm::Neutral;
		}

		const int prevAmmo = mp->mNowBullet;
		const uint8 comboStepAtFire = (button == InputButtons::ATTACK) ? mp->mComboStep : 0;
		const uint32 currentActionState = mp->GetState();
		SkillType bulletType = ResolveSkillType(mp->mPlayerType, button, rhythm);
		if (button == InputButtons::ATTACK &&
			mp->mPlayerType == Rudwig &&
			(comboStepAtFire >= 2 || currentActionState == S_ComboAttack1))
		{
			bulletType = SkillType::DrumAttack3;
		}
		if (button == InputButtons::ATTACK &&
			mp->mPlayerType == Ibanix)
		{
			if (comboStepAtFire == 1)
				bulletType = SkillType::BaseAttack2;
			else if (comboStepAtFire >= 2)
				bulletType = SkillType::BaseAttack3;
		}
		EnqueueAttackEventByCategory(em, e, bulletType, isCritical);

			switch (button)
			{
			case InputButtons::ATTACK:
				if (mp->mPlayerType == Ibanix)
				{
					const int ammoCost = (bulletType == SkillType::BaseAttack2) ? 3 : 1;
					mp->mNowBullet = (std::max)(0, mp->mNowBullet - ammoCost);
				}
				else if (mp->mPlayerType == Fanthor && bulletType != SkillType::GuitarAttack)
					mp->mNowBullet = (std::max)(0, mp->mNowBullet - 1);
				break;
	case InputButtons::SKILL1:
		if (mp->mPlayerType == Ibanix)
			mp->mNowBullet = (std::max)(0, mp->mNowBullet - 1);
		break;
	default:
		break;
	}

	if (!(delaySkill1Attack && !mp->mStateThrew))
		mp->mFsm.ChangeState(mp, nextState);

	if (delaySkill1Attack)
		mp->mStateThrew = true;
	*nextTimePtr = now + Beat * cool;
	// 쿨이 실제로 도는 경로에서만 1회 통지
	em.Enqueue<EvCooldownStarted>({ e, cdSlot, Beat * cool });

	if (button == InputButtons::ATTACK)
	{
		if (nextState == Attack1State::Instance())
			mp->mComboStep = 1;
		else if (nextState == ComboAttack1State::Instance())
			mp->mComboStep = 2;
		else
			mp->mComboStep = 0;
	}
	EnqueueAmmoChangedIfNeeded(mWorld, em, e, prevAmmo);
	return true;
}
