#pragma once
#include "Component.h"
#include "TransformComponent.h"
#include "StateMachine.h"
#include "System.h"

enum PlayerType : uint8
{
	Rudwig,
	Ibanix,
	Fanthor,
	Count,
};

class ControllerComponent : public Component<ControllerComponent>
{
public:
	ControllerComponent() : mTransformComponent(){}
	ControllerComponent(TransformComponent transform): mTransformComponent(transform){}
	ControllerComponent(TransformComponent transform, int mode) : mTransformComponent(transform), mPlayMode(mode) {}

	//void RegisterEditorProperties(std::vector<EditorProperty>& out)
	//{
	//	out.push_back({ "Speed", PropertyType::Float, &mCameraHight, 0.0f, 10.0f });
	//	out.push_back({ "GodMode", PropertyType::Bool, &mCameraLenth });
	//}

public:
	TransformComponent mTransformComponent;
	int mPlayMode = 0;
	float mCameraHight = 1.f; 
	float mCameraLenth = 5.f; 
};

//------------------------------------------------------------------------------------------------

static std::unordered_map<std::string, uint64_t> gFlagByName = {
	{"F_NONE", 1ull << 0},
	{"F_MOVE", 1ull << 1} ,{"F_STUN", 1ull << 2}, {"F_DEAD", 1ull << 3},{"F_JUMP", 1ull << 4}, {"F_SA", 1ull << 5}, {"F_INVUL", 1ull <<6},
	{"F_NO_RUN", 1ull <<7}
};

enum : StateId {
	S_Idle = 0, S_RunForward, S_RunBackward, S_RunRight, S_RunLeft,
	S_Jump, S_Fall, S_Land, S_Dash,
	S_Attack1, S_Attack2, S_Skill1, S_Skill2, S_Special,
	S_Reload, S_RhythmChange, S_Aim,
	S_Dead, S_ComboAttack1, S_ComboAttack2, S_Hit, S_Stun, S_Dance1,

};

enum PlayerFlags : uint64_t
{
	FLAG_NONE = 1ull << 0, //상시 꺼짐 전이 불가 조건
	FLAG_ANIM = 1ull << 1,
	FLAG_MOVE = 1ull << 2,
	FLAG_STUN = 1ull << 3,
	FLAG_DEAD = 1ull << 4,
	FLAG_JUMP = 1ull << 5,
	FLAG_SA	  = 1ull << 6,
	FLAG_INVUL =1ull << 7,
	FLAG_NO_RUN=1ull << 8,
};

enum class PlayerDeathCause : uint8
{
	None = 0,
	Fall,
	Health
};

inline void SetFlag(uint64_t& f, uint64_t m) { f |= m; }   // 켜기
inline void ClearFlag(uint64_t& f, uint64_t m) { f &= ~m; }   // 끄기

enum class PendingAction : uint8_t
{
	None,
	Attack,
	Skill1,
	Skill2,
	Reload
};

// 플레이어별 FSM State
struct PlayerStateTiming
{
	bool  animOnce = false;
	float stateTime = 3.0f;
	float animEndTime = 0.0f;
};

class MainPlayerComponent : public Component<MainPlayerComponent>
{
public:
	MainPlayerComponent();
	MainPlayerComponent(const std::string& path, PlayerType playerType);

	void StateCheck();
	void Update(float dt);


	void InitFSMOnce();
	void InitFSMFromJson(const std::string& path);
	void LoadStateSettingFromJson(const std::string& path);
public:
	uint32 GetState() { return (uint32)mFsm.GetState(); };
	uint8 GetReplicatedActionState();
	uint8 GetReplicatedMovementMode();
	uint8 GetReplicatedControlFlags();
	uint8 GetReplicatedExternalMoveMode();
	bool CanUseHorizontalInput();
	bool CanUseVerticalInput();
	bool IsDeathActive() const { return mDeathCause != PlayerDeathCause::None; }


public:
	PlayerType mPlayerType = PlayerType::Fanthor;
public:
	PendingAction mPendingAction = PendingAction::None;
	bool mStateThrew = true;
	float mSkill1PendingActionTime = 0.0f;
	StateMachine<MainPlayerComponent> mFsm{this};
	int mNextState = 0;

	//  플레이어별 상태 타이밍
	static constexpr int kPlayerStateCount = 32;
	std::array<PlayerStateTiming, kPlayerStateCount> mStateTimings{};

	Vec2 mPlayerMovingDir = Vec2::Zero;

	Vec3 mSpawnPosition = Vec3::Zero;	// 낙사 후 되돌릴 스폰 위치이다.
	PlayerDeathCause mDeathCause = PlayerDeathCause::None; // 사망 원인
	float mDeathStartTime = 0.0f;
	float mDeathEndTime = 0.0f;			// 리스폰 쿨타임

	bool  mRespawnVfxPending = false;	// 스폰 VFX 방송 후 실제 부활을 대기 중인지
	float mRespawnReviveTime = 0.0f;	// 실제 부활을 수행할 서버 시각

public:
	//float mWalkSpeed = 0.0f;
	float mRunSpeed = 0.0f;
	float mDashSpeed = 0.0f;
	float mDashTime = 3.0f;

	float mAttackCool= 2.f;
	float mSkill1Cool= 2.f;
	float mSkill2Cool= 2.f;
	float mReloadCool= 2.f;


	float mRhythmEffectRadius = 1500.f;	// 기본 리듬 효과 제공 반경( 0 이하 전범위)

public:
	float mNextAttackTime = 1.f;
	float mNextSkill1Time = 1.f;
	float mNextSkill2Time = 1.f;
	float mNextReloadTime = 1.f;
	float mNextRythmChangeTime = 1.f;

	uint8 mLastBeatJudgement = 0; // 마지막 행동의 박자 판정(BeatJudgement)
	float mLastJudgedInputSongPos = -1.f; // 동일 입력 중복 판정 방지

	static constexpr int32 kMaxRhythmPoints = 200;
	int32 mRhythmPoints = 0;

	bool mBaseUltimateActive = false;
	float mBaseUltimateEndTime = 0.0f;
	float mBaseUltimateRemainingTime = 0.0f;
	float mBaseUltimateNextTickTime = 0.0f;
	int32 mBaseUltimateTicksRemaining = 0;
	bool mSpecialButtonWasDown = false;


	float mComboExpireTime = 0.0f;
	float mComboInputWindow = 0.75f;
	uint8 mComboStep = 0;

	// 리듬 콤보 Count (적을 피격했을 때 증가하는 콤보)
	int32 mRhythmCombo = 0;                  // 현재 콤보 수
	float mRhythmComboExpireTime = 0.0f;     // 이 서버 시각(초)을 넘기면 콤보 만료
	static constexpr float kRhythmComboTimeout = 3.0f; // 무공격 허용 시간(초)

	float mComboHitEligibleUntil = 0.0f;	// 콤보 인정시간
	static constexpr float kComboHitWindow = 1.5f; // 투사체 비행시간을 감안한 인정 시간

	// 콤보로 인정된 적 목록
	static constexpr uint8 kComboHitTargetsMax = 8;
	std::array<uint32, kComboHitTargetsMax> mComboHitTargets{};
	uint8 mComboHitTargetCount = 0;


	float mSpeed = 0.0f;

	float mJumpPower = 360.f;
	bool mFalling = false;
	bool mDash = false;
	bool mHasMoveInput = false;					// 플레이어가 현재 이동 입력을 넣고 있는지
	bool mCanControlHorizontal = true;			// 플레이어가 XZ 평면 이동을 조작할 수 있는지
	bool mCanControlVertical = true;			// 플레이어가 수직 조작을 할 수 있는지
	uint8 mExternalMoveMode = static_cast<uint8>(ReplicatedExternalMoveMode::None);	// 플레이어 이동에 외부 힘이 개입하고 있는지
	Vec3 mExternalVelocity = Vec3::Zero;		// 외부 힘으로 발생한 이동 속도
	float mExternalMoveEndTime = 0.0f;			// 외부 이동 상태가 언제 끝나는지

	uint64_t mFlags = 0ull;
	bool mAnimEnd = false;
	float mStateEnd = 0.0f;
	float mDashEnd = 0.0f;
	float mDt = 0.0f;
	uint32 mStateSequence = 0;

		int mMaxBullet;
		int mNowBullet;
		int mLastReplicatedAmmo = -1;

};

//player base --------------------------------------------------
class IdleState : public State<MainPlayerComponent> {
public:
	static IdleState* Instance();
	virtual const char* GetName() const override { return "IdleState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RunForwardState : public State<MainPlayerComponent> {
public:
	static RunForwardState* Instance();
	virtual const char* GetName() const override { return "RunForwardState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RunBackwardState : public State<MainPlayerComponent> {
public:
	static RunBackwardState* Instance();
	virtual const char* GetName() const override { return "RunBackwardState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RunRightState : public State<MainPlayerComponent> {
public:
	static RunRightState* Instance();
	virtual const char* GetName() const override { return "RunRightState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RunLeftState : public State<MainPlayerComponent> {
public:
	static RunLeftState* Instance();
	virtual const char* GetName() const override { return "RunLeftState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
//class RunState : public State<MainPlayerComponent> {
//public:
//	static RunState* Instance();
//	virtual const char* GetName() const override { return "RunState"; }
//	void Enter(MainPlayerComponent* owner) override;
//	void Update(MainPlayerComponent* owner) override;
//	void Exit(MainPlayerComponent* owner) override;
//};
class JumpState : public State<MainPlayerComponent> {
public:
	static JumpState* Instance();
	virtual const char* GetName() const override { return "JumpState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class FallState : public State<MainPlayerComponent> {
public:
	static FallState* Instance();
	virtual const char* GetName() const override { return "FallState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class LandState : public State<MainPlayerComponent> {
public:
	static LandState* Instance();
	virtual const char* GetName() const override { return "LandState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class DashState : public State<MainPlayerComponent> {
public:
	static DashState* Instance();
	virtual const char* GetName() const override { return "DashState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//battle support------------------------------------------------
class AimState : public State<MainPlayerComponent> {
public:
	static AimState* Instance();

	virtual const char* GetName() const override { return "AimState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class ReloadState : public State<MainPlayerComponent> {
public:
	static ReloadState* Instance();

	virtual const char* GetName() const override { return "ReloadState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class RhythmChangeState : public State<MainPlayerComponent> {
public:
	static RhythmChangeState* Instance();

	virtual const char* GetName() const override { return "RhythmChangeState"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Dance1State : public State<MainPlayerComponent> {
public:
	static Dance1State* Instance();

	virtual const char* GetName() const override { return "Dance1State"; }

	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//damage--------------------------------------------------------
class HitState : public State<MainPlayerComponent> {
public:
	static HitState* Instance();
	virtual const char* GetName() const override { return "HitState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class StunState : public State<MainPlayerComponent> {
public:
	static StunState* Instance();
	virtual const char* GetName() const override { return "StunState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class DeadState : public State<MainPlayerComponent> {
public:
	static DeadState* Instance();
	virtual const char* GetName() const override { return "DeadState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};

//attack--------------------------------------------------------
class Attack1State : public State<MainPlayerComponent> {
public:
	static Attack1State* Instance();
	virtual const char* GetName() const override { return "Attack1State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Attack2State : public State<MainPlayerComponent> {
public:
	static Attack2State* Instance();
	virtual const char* GetName() const override { return "Attack2State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class ComboAttack1State : public State<MainPlayerComponent> {
public:
	static ComboAttack1State* Instance();
	virtual const char* GetName() const override { return "ComboAttack1State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class ComboAttack2State : public State<MainPlayerComponent> {
public:
	static ComboAttack2State* Instance();
	virtual const char* GetName() const override { return "ComboAttack2State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill1State : public State<MainPlayerComponent> {
public:
	static Skill1State* Instance();
	virtual const char* GetName() const override { return "Skill1State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class Skill2State : public State<MainPlayerComponent> {
public:
	static Skill2State* Instance();
	virtual const char* GetName() const override { return "Skill2State"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
class SpecialState : public State<MainPlayerComponent> {
public:
	static SpecialState* Instance();
	virtual const char* GetName() const override { return "SpecialState"; }
	void Enter(MainPlayerComponent* owner) override;
	void Update(MainPlayerComponent* owner) override;
	void Exit(MainPlayerComponent* owner) override;
};
