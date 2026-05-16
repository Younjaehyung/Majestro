#include "pch.h"
#include "json.hpp"

using json = nlohmann::json;
#include "PlayerComponent.h"
#include "StateMachine.h"

BOOL STATE_DEBUG = TRUE;
std::vector<State<MainPlayerComponent>*> mStateList;

static StateId NameToId(const std::string& n) {
    if (n == "Idle") return S_Idle;
    if (n == "RunForward") return S_RunForward;
    if (n == "RunBackward") return S_RunBackward;
    if (n == "RunRight") return S_RunRight;
    if (n == "RunLeft") return S_RunLeft;
    //if (n == "Run")  return S_Run;
    if (n == "Jump")  return S_Jump;
    if (n == "Fall")  return S_Fall;
    if (n == "Land")  return S_Land;
    if (n == "Dash")  return S_Dash;

    if (n == "Aim")  return S_Aim;
    if (n == "Reload")  return S_Reload;
    if (n == "RhythmChange")  return S_RhythmChange;

    if (n == "Hit")  return S_Hit;
    if (n == "Stun")  return S_Stun;
    if (n == "Dead")  return S_Dead;

    if (n == "Attack1")  return S_Attack1;
    if (n == "Attack2")  return S_Attack2;
    if (n == "Skill1")  return S_Skill1;
    if (n == "Skill2")  return S_Skill2;
    if (n == "Special")  return S_Special;

    return 255;
}

MainPlayerComponent::MainPlayerComponent() : mFsm(this), mSpeed(0.0f), mFlags(0ull)
{
}

MainPlayerComponent::MainPlayerComponent(const std::string& path) : mFsm(this), mSpeed(0.0f), mFlags(0ull) 
{
    mStateList = {
    IdleState::Instance(),
    WalkState::Instance(),
    RunState::Instance(),
    JumpState::Instance(),
    FallState::Instance(),
    LandState::Instance(),
    DashState::Instance(),

    Attack1State::Instance(),

    AimState::Instance(),
    ReRoadState::Instance(),
    RhythmChangeState::Instance(),

    HitState::Instance(),
    StunState::Instance(),
    DeadState::Instance(),

    /*Attack1State::Instance(),*/
    Attack2State::Instance(),
    Skill1State::Instance(),
    Skill2State::Instance(),
    SpecialState::Instance()
    };

    //InitFSMFromJson(path);
    //LoadStateSettingFromJson("../Resources/Json/StateSetting.json");
};

MainPlayerComponent::MainPlayerComponent(const std::string& path, vector<shared_ptr<Animator>> anim) : mFsm(this), mSpeed(0.0f), mFlags(0ull) {
    mStateList = {
    IdleState::Instance(),
    WalkState::Instance(),
    RunState::Instance(),
    JumpState::Instance(),
    FallState::Instance(),
    LandState::Instance(),
    DashState::Instance(),

    Attack1State::Instance(),

    AimState::Instance(),
    ReRoadState::Instance(),
    RhythmChangeState::Instance(),

    HitState::Instance(),
    StunState::Instance(),
    DeadState::Instance(),

    /*Attack1State::Instance(),*/
    Attack2State::Instance(),
    Skill1State::Instance(),
    Skill2State::Instance(),
    SpecialState::Instance()
    };
    //InitFSMFromJson(path);
    //LoadStateSettingFromJson("../Resources/Json/StateSetting.json");

    for (int i = 0; i < (int)anim.size(); i++)
    {
        cout << "?" << endl;
        mStateList[i]->mAnimEndTime = static_cast<float>(anim[i]->mEndTime);
        cout << "State[" << i << "] EndTime = " << static_cast<float>(anim[i]->mEndTime)  << " : " << mStateList[i]->mAnimEndTime << endl;
    }


}

MainPlayerComponent::MainPlayerComponent(const std::string& path, vector<shared_ptr<Animator>> anim, PlayerType playerType) : mFsm(this), mSpeed(0.0f), mFlags(0ull), mPlayerType(playerType)
{
    mStateList = {
    IdleState::Instance(),
    WalkState::Instance(),
    RunState::Instance(),
    JumpState::Instance(),
    FallState::Instance(),
    LandState::Instance(),
    DashState::Instance(),

    Attack1State::Instance(),
    Attack2State::Instance(),
    Skill1State::Instance(),
    Skill2State::Instance(),
    SpecialState::Instance(),

    AimState::Instance(),
    ReRoadState::Instance(),
    RhythmChangeState::Instance(),

    HitState::Instance(),
    StunState::Instance(),
    DeadState::Instance()

    };
    //InitFSMFromJson(path);
    //LoadStateSettingFromJson("../Resources/Json/StateSetting.json");

    for (int i = 0; i < (int)anim.size(); i++)
    {
        mStateList[i]->mAnimEndTime = static_cast<float>(anim[i]->mEndTime);
        cout << "State[" << i << "] EndTime = " << static_cast<float>(anim[i]->mEndTime) << " : " << mStateList[i]->mAnimEndTime << endl;
    }
}

void MainPlayerComponent::StateCheck()
{
    if(mSpeed<1.f)ClearFlag(mFlags, FLAG_MOVE);
    else SetFlag(mFlags, FLAG_MOVE);

    if (mFalling) {
        mFsm.ChangeState(this, FallState::Instance());
    }

}

void MainPlayerComponent::Update(float dt) 
{
    //mStateTime += dt;
    //mDt = dt;
    //StateCheck();
    //mFsm.Update(this);
}

void MainPlayerComponent::InitFSMOnce()
{
}

void MainPlayerComponent::InitFSMFromJson(const std::string& path)
{
    cout << "json input" << endl;

    // 1) 포인터→ID 변환기 주입 (상태 이름→StateId)
    mFsm.SetIdResolver([](State<MainPlayerComponent>* s)->StateId {
        if (s == IdleState::Instance()) return S_Idle;
        //if (s == WalkState::Instance()) return S_Walk;
        //if (s == RunState::Instance())  return S_Run;
        if (s == JumpState::Instance()) return S_Jump;
        if (s == FallState::Instance()) return S_Fall;
        if (s == LandState::Instance()) return S_Land;
        if (s == DashState::Instance()) return S_Dash;

        if (s == Attack1State::Instance()) return S_Attack1;
        if (s == Attack2State::Instance()) return S_Attack2;
        if (s == Skill1State::Instance()) return S_Skill1;
        if (s == Skill2State::Instance()) return S_Skill2;
        if (s == SpecialState::Instance()) return S_Special;
        return 255;
        });

    // 2) JSON 열기
    std::ifstream ifs(path);
    if (!ifs) {
        cout << "non json" << endl;
        InitFSMOnce();
        return;
    }

    json j;
    ifs >> j;

    // 3) flags 병합
    if (j.contains("flags") && j["flags"].is_object()) {
        for (auto it = j["flags"].begin(); it != j["flags"].end(); ++it) {
            gFlagByName[it.key()] = static_cast<uint64_t>(it.value());
        }
    }

    // 4) require/forbid 파서
    auto toMask = [](const json& arr)->uint64_t {
        uint64_t m = 0;
        for (auto& v : arr) {
            const std::string name = v.get<std::string>();
            auto it = gFlagByName.find(name);
            if (it != gFlagByName.end())
                m |= it->second;
        }
        return m;
        };

    // 5) guards 적용 (speed 제거 버전)
    if (j.contains("guards") && j["guards"].is_array()) {
        for (auto& g : j["guards"]) {

            // 상태 이름 → ID
            const std::string fromName = g["from"].get<std::string>();
            const std::string toName = g["to"].get<std::string>();

            StateId fromId = NameToId(fromName);
            StateId toId = NameToId(toName);

            if (fromId == 255 || toId == 255)
                continue;

            // speed 조건 제거 -> require / forbid만 가져간다
            uint64_t reqMask = g.contains("require") ? toMask(g["require"]) : 0ull;
            uint64_t forbidMask = g.contains("forbid") ? toMask(g["forbid"]) : 0ull;

            // FSM에 guard 등록
            mFsm.AddGuardById(fromId, toId,
                [reqMask, forbidMask](MainPlayerComponent* o)
                {
                    // require 플래그 충족?
                    if ((o->mFlags & reqMask) != reqMask)
                        return false;

                    // forbid 플래그 존재하면 실패
                    if ((o->mFlags & forbidMask) != 0)
                        return false;

                    return true;
                }
            );
        }
    }

    // 6) 초기 상태 진입
    mFsm.ChangeState(this, IdleState::Instance());
}

void MainPlayerComponent::LoadStateSettingFromJson(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) {
        std::cout << "stateProps json not found\n";
        return;
    }
    json j;
    ifs >> j;

    // 1) PLAYER 기본 정보 로딩
    if (j.contains("player"))
    {
        auto& p = j["player"];

        if (p.contains("walkSpeed"))
            mWalkSpeed = p["walkSpeed"].get<float>();

        if (p.contains("runSpeed"))
            mRunSpeed = p["runSpeed"].get<float>();

        if (p.contains("dashSpeed"))
            mDashSpeed = p["dashSpeed"].get<float>();

        //if (p.contains("jumpForce"))
            //mJumpForce = p["jumpForce"].get<float>();

        std::cout << "[Player Config Loaded]\n";
        std::cout << "  WalkSpeed : " << mWalkSpeed << "\n";
        std::cout << "  RunSpeed  : " << mRunSpeed << "\n";
        std::cout << "  DashSpeed : " << mDashSpeed << "\n";
    }
    // 2) STATE PROPERTY 로딩
    if (!j.contains("stateProps"))
        return;

    auto& props = j["stateProps"];

    for (auto* st : mStateList)
    {
        const char* name = st->GetName();

        if (!props.contains(name))
            continue;

        auto& _p = props[name];

        if (_p.contains("time"))
            st->mStateTime = _p["time"].get<float>();

        if (_p.contains("animOnce"))
            st->mAnimOnce = _p["animOnce"].get<bool>();
        
    }

    std::cout << "[State Props Loaded]\n";
}

//---------------------------------------------------------------------------------------------------
void StateEnter(State<MainPlayerComponent>* s, MainPlayerComponent* owner)
{
    owner->mStateTime = 0.0f;
    owner->mNextState = S_Idle;
    if (STATE_DEBUG) { std::cout << "Enter " << s->GetName() <<"\n"; }
    if(s->mAnimOnce) SetFlag(owner->mFlags, FLAG_ANIM);
}

void StateUpdate(State<MainPlayerComponent>* s, MainPlayerComponent* owner) {
    if (s->mAnimOnce && owner->mStateTime >= s->mAnimEndTime) {
        if (s->mAnimOnce) ClearFlag(owner->mFlags, FLAG_ANIM);
        owner->mFsm.ChangeState(owner, mStateList[owner->mNextState]);
    }

}

void StateExit(State<MainPlayerComponent>* s, MainPlayerComponent* owner)
{
    if (STATE_DEBUG) { std::cout << "Exit " << s->GetName() << "\n"; }
}



IdleState* IdleState::Instance() {
    static IdleState inst;
    return &inst;
}
void IdleState::Enter(MainPlayerComponent* owner) 
{
    ClearFlag(owner->mFlags, FLAG_MOVE);
    StateEnter(this, owner);
}
void IdleState::Update(MainPlayerComponent* owner) {
    
}
void IdleState::Exit(MainPlayerComponent* owner) {
    StateExit(this, owner);
}

WalkState* WalkState::Instance() {
    static WalkState inst;
    return &inst;
}
void WalkState::Enter(MainPlayerComponent* owner) 
{
    SetFlag(owner->mFlags, FLAG_MOVE);
    StateEnter(this, owner);
}
void WalkState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
    if(owner->mSpeed <=0.f) owner->mFsm.ChangeState(owner, IdleState::Instance());

    if (owner->mFlags & FLAG_NO_RUN) { if (owner->mSpeed > owner->mRunSpeed) owner->mSpeed = owner->mWalkSpeed; } //달리기 불가 시 속도 강제 다운
    else if(owner->mSpeed >= owner->mRunSpeed) owner->mFsm.ChangeState(owner, RunState::Instance());
}
void WalkState::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

RunState* RunState::Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
    static RunState inst;                          // 최초 호출 시 한 번만 생성
    return &inst;
}
void RunState::Enter(MainPlayerComponent* owner)
{
    SetFlag(owner->mFlags, FLAG_MOVE);
    StateEnter(this, owner);
}
void RunState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
    if (owner->mSpeed < owner->mRunSpeed) owner->mFsm.ChangeState(owner, WalkState::Instance());
}
void RunState::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

JumpState* JumpState::Instance() {                 
    static JumpState inst;                          
    return &inst;
}
void JumpState::Enter(MainPlayerComponent* owner) {
    StateEnter(this,owner);
    SetFlag(owner->mFlags, FLAG_JUMP);
    owner->mNextState = S_Fall;
}
void JumpState::Update(MainPlayerComponent* owner) {
    StateUpdate(this, owner);
}
void JumpState::Exit(MainPlayerComponent* owner) {
    //owner->mFalling = true;
    StateExit(this, owner);
}

FallState* FallState::Instance() {
    static FallState inst;
    return &inst;
}
void FallState::Enter(MainPlayerComponent* owner) {
    StateEnter(this, owner);
    SetFlag(owner->mFlags, FLAG_JUMP);
}
void FallState::Update(MainPlayerComponent* owner) {
    StateUpdate(this, owner);
    if (not owner->mFalling) {
        ClearFlag(owner->mFlags, FLAG_JUMP);
        owner->mFsm.ChangeState(owner, LandState::Instance());
    }
}
void FallState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

LandState* LandState::Instance() {
    static LandState inst;
    return &inst;
}
void LandState::Enter(MainPlayerComponent* owner) {
    StateEnter(this, owner);
}
void LandState::Update(MainPlayerComponent* owner) {
    StateUpdate(this, owner);
}
void LandState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

DashState* DashState::Instance() {
    static DashState inst;
    return &inst;
}
void DashState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void DashState::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void DashState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

//battle support-----------------------------------------------------
AimState* AimState::Instance() {
    static AimState inst;
    return &inst;
}
void AimState::Enter(MainPlayerComponent* owner)
{
    StateEnter(this, owner);
}
void AimState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
}
void AimState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

ReRoadState* ReRoadState::Instance() {
    static ReRoadState inst;
    return &inst;
}
void ReRoadState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void ReRoadState::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void ReRoadState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

RhythmChangeState* RhythmChangeState::Instance() {
    static RhythmChangeState inst;
    return &inst;
}
void RhythmChangeState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void RhythmChangeState::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void RhythmChangeState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

//damage-------------------------------------------------------------
HitState* HitState::Instance() {
    static HitState inst;
    return &inst;
}
void HitState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void HitState::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void HitState::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

StunState* StunState::Instance() {
    static StunState inst;
    return &inst;
}
void StunState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void StunState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
}
void StunState::Exit(MainPlayerComponent* owner)
{
    StateExit(this, owner);
}

DeadState* DeadState::Instance() {
    static DeadState inst;
    return &inst;
}
void DeadState::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void DeadState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
}
void DeadState::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}


//attack-------------------------------------------------------------
Attack1State* Attack1State::Instance() {
    static Attack1State inst;
    return &inst;
}
void Attack1State::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void Attack1State::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void Attack1State::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

Attack2State* Attack2State::Instance() {
    static Attack2State inst;
    return &inst;
}
void Attack2State::Enter(MainPlayerComponent* owner) 
{
    StateEnter(this, owner);
}
void Attack2State::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
}
void Attack2State::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

Skill1State* Skill1State::Instance() {
    static Skill1State inst;
    return &inst;
}
void Skill1State::Enter(MainPlayerComponent* owner)
{
    StateEnter(this, owner);
}
void Skill1State::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void Skill1State::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

Skill2State* Skill2State::Instance() {
    static Skill2State inst;
    return &inst;
}
void Skill2State::Enter(MainPlayerComponent* owner)
{
    StateEnter(this, owner);
}
void Skill2State::Update(MainPlayerComponent* owner) 
{
    StateUpdate(this, owner);
}
void Skill2State::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}

SpecialState* SpecialState::Instance() {
    static SpecialState inst;
    return &inst;
}
void SpecialState::Enter(MainPlayerComponent* owner)
{
    StateEnter(this, owner);
}
void SpecialState::Update(MainPlayerComponent* owner)
{
    StateUpdate(this, owner);
}
void SpecialState::Exit(MainPlayerComponent* owner) 
{
    StateExit(this, owner);
}
