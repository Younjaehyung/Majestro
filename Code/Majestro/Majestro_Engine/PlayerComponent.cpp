#include "pch.h"
#include "json.hpp"
#include <fstream>
#include <limits>
using json = nlohmann::json;
#include "PlayerComponent.h"
#include "StateMachine.h"

BOOL STATE_DEBUG = FALSE;
std::vector<State<MainPlayerComponent>*> mStateList;

static StateId NameToId(const std::string& n) {
	if (n == "Idle") return S_Idle;
    if (n == "Walk") return S_Walk;
	if (n == "Run")  return S_Run;
	if (n == "Jump")  return S_Jump;
	if (n == "Dash")  return S_Dash;

    if (n == "Aim")  return S_Aim;
    if (n == "ReRoad")  return S_ReRoad;
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

    InitFSMFromJson(path);
    LoadStateSettingFromJson("../Resources/Json/StateSetting.json");
};

MainPlayerComponent::MainPlayerComponent(const std::string& path, vector<shared_ptr<Animator>> anim) : mFsm(this), mSpeed(0.0f), mFlags(0ull) {
    mStateList = {
    IdleState::Instance(),
    WalkState::Instance(),
    RunState::Instance(),
    JumpState::Instance(),
    DashState::Instance(),

    AimState::Instance(),
    ReRoadState::Instance(),
    RhythmChangeState::Instance(),

    HitState::Instance(),
    StunState::Instance(),
    DeadState::Instance(),

    Attack1State::Instance(),
    Attack2State::Instance(),
    Skill1State::Instance(),
    Skill2State::Instance(),
    SpecialState::Instance()
    };
    InitFSMFromJson(path);
    LoadStateSettingFromJson("../Resources/Json/StateSetting.json");

    for (int i = 0; i < (int)anim.size(); i++)
    {
        mStateList[i]->mAnimEndTime = static_cast<float>(anim[i]->mEndTime);
        cout << "State[" << i << "] EndTime = " << mStateList[i]->mAnimEndTime << endl;
    }
}

void MainPlayerComponent::StateCheck()
{
    if(mSpeed<30.f)ClearFlag(mFlags, FLAG_MOVE);
    if (mHight <= mGround) {
        mHight = mGround;
        mGravity = 0.0f;
        ClearFlag(mFlags, FLAG_JUMP);
    }
    else {
        mGravity += mGravityA * mDt;
        mHight -= mGravity;
    }

}

void MainPlayerComponent::Update(float dt) 
{
    mStateTime += dt;
    mDt = dt;
    StateCheck();
    mFsm.Update(this);
}

void MainPlayerComponent::InitFSMOnce()
{
}

void MainPlayerComponent::InitFSMFromJson(const std::string& path)
{
    cout << "json input" << endl;

    // 1) 포인터→ID 변환기 주입 (상태 이름/ID와 1:1로 일치)
    mFsm.SetIdResolver([](State<MainPlayerComponent>* s)->StateId {
        if (s == IdleState::Instance()) return S_Idle;
        if (s == WalkState::Instance())  return S_Walk;
        if (s == RunState::Instance())  return S_Run;
        if (s == JumpState::Instance())  return S_Jump;
        if (s == DashState::Instance())  return S_Dash;
        return 255;
        });

    // 2) JSON 열기
    std::ifstream ifs(path);
    if (!ifs) {
        // 파일 없으면 하드코딩 초기화로 폴백하거나, 안전하게 return
        cout << "non json" << endl;
        InitFSMOnce(); // ← 필요하면 이렇게 폴백
        return;
    }

    json j; 
    ifs >> j;

    // 3) flags 섹션이 있으면 병합 (외부에서 비트값을 지정 가능)
    if (j.contains("flags") && j["flags"].is_object()) {
        for (auto it = j["flags"].begin(); it != j["flags"].end(); ++it) {
            gFlagByName[it.key()] = static_cast<uint64_t>(it.value());
        }
    }

    // 4) guards 읽어서 fsm.AddGuardById 로 등록
    const float INF = std::numeric_limits<float>::infinity();

    auto toMask = [](const json& arr)->uint64_t {
        uint64_t m = 0;
        for (auto& v : arr) {
            const std::string name = v.get<std::string>();
            auto it = gFlagByName.find(name);
            if (it != gFlagByName.end()) m |= it->second;
        }
        return m;
        };

    if (j.contains("guards") && j["guards"].is_array()) {
        for (auto& g : j["guards"]) {
            // 상태 이름 → ID
            const std::string fromName = g["from"].get<std::string>();
            const std::string toName = g["to"].get<std::string>();
            StateId fromId = NameToId(fromName);
            StateId toId = NameToId(toName);
            if (fromId == 255 || toId == 255) continue; // 알 수 없는 상태 스킵

            // 속도 범위
            float minS = -INF, maxS = +INF;
            if (g.contains("speed") && g["speed"].is_array() && g["speed"].size() == 2) {
                const auto& a = g["speed"];
                if (!a[0].is_null()) minS = a[0].get<float>();
                if (!a[1].is_null()) maxS = a[1].get<float>();
            }

            // require/forbid 마스크
            uint64_t reqMask = g.contains("require") ? toMask(g["require"]) : 0ull;
            uint64_t forbidMask = g.contains("forbid") ? toMask(g["forbid"]) : 0ull;

            // 가드 람다 등록
            mFsm.AddGuardById(fromId, toId,
                [minS, maxS, reqMask, forbidMask](MainPlayerComponent* o) {
                    if (o->mSpeed < minS || o->mSpeed > maxS) return false;
                    if ((o->mFlags & reqMask) != reqMask)     return false;
                    if ((o->mFlags & forbidMask) != 0)        return false;
                    return true;
                });
        }
    }

    // 5) 초기 상태 진입 (Enter 호출 시점이 민감하면 첫 Update 때로 미뤄도 됨)
    mFsm.ChangeState(this,IdleState::Instance());
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

    if (!j.contains("stateProps")) return;

    auto& props = j["stateProps"];

    for (auto* st : mStateList)
    {
        const char* name = st->GetName();

        if (props.contains(name))
        {
            if (props[name].contains("time"))
                st->mStateTime = props[name]["time"];
        }
    }

}


//---------------------------------------------------------------------------------------------------

IdleState* IdleState::Instance() {
    static IdleState inst;
    return &inst;
}
void IdleState::Enter(MainPlayerComponent* owner) {
    ClearFlag(owner->mFlags, FLAG_MOVE);
    if(STATE_DEBUG) std::cout << "Enter Idle\n";
}
void IdleState::Update(MainPlayerComponent* owner) {

}
void IdleState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Idle\n";
}

WalkState* WalkState::Instance() {
    static WalkState inst;
    return &inst;
}
void WalkState::Enter(MainPlayerComponent* owner) {
    SetFlag(owner->mFlags, FLAG_MOVE);
    if(STATE_DEBUG) std::cout << "Enter Walk\n";
}
void WalkState::Update(MainPlayerComponent* owner) {
    owner->mFsm.ChangeState(owner, IdleState::Instance());

    if (owner->mFlags & FLAG_NO_RUN) { if (owner->mSpeed > 70.0f) owner->mSpeed = 69.9f; } //달리기 불가 시 속도 강제 다운
    else owner->mFsm.ChangeState(owner, RunState::Instance());
}
void WalkState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Walk\n";
}

RunState* RunState::Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
    static RunState inst;                          // 최초 호출 시 한 번만 생성
    return &inst;
}
void RunState::Enter(MainPlayerComponent* owner) {
    SetFlag(owner->mFlags, FLAG_MOVE);
    if(STATE_DEBUG) std::cout << "Enter Run\n";
}
void RunState::Update(MainPlayerComponent* owner) {
    if (owner->mFsm.ChangeState(owner, WalkState::Instance())) return;
}
void RunState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Run\n";
}

JumpState* JumpState::Instance() {                 
    static JumpState inst;                          
    return &inst;
}
void JumpState::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0.0f;
    owner->mHight = owner->mGround+ 0.1f;
    SetFlag(owner->mFlags, FLAG_JUMP);
    if(STATE_DEBUG) std::cout << "Enter Jump\n";
}
void JumpState::Update(MainPlayerComponent* owner) {
    if (owner->mFsm.ChangeState(owner, IdleState::Instance())) return;
    owner->mHight += owner->mJumpPower * owner->mDt;
    //cout << owner->mHight << endl;
}
void JumpState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Jump\n";
}

DashState* DashState::Instance() {
    static DashState inst;
    return &inst;
}
void DashState::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0.0f;
    if(STATE_DEBUG) std::cout << "Enter Dash\n";
}
void DashState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        cout << this->mStateTime << endl;
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void DashState::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Dash\n";
}

//battle support-----------------------------------------------------
AimState* AimState::Instance() {
    static AimState inst;
    return &inst;
}
void AimState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter Aim\n";
}
void AimState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void AimState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Aim\n";
}

ReRoadState* ReRoadState::Instance() {
    static ReRoadState inst;
    return &inst;
}
void ReRoadState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter Reroad\n";
}
void ReRoadState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void ReRoadState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Reroad\n";
}

RhythmChangeState* RhythmChangeState::Instance() {
    static RhythmChangeState inst;
    return &inst;
}
void RhythmChangeState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter RhythmChange\n";
}
void RhythmChangeState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void RhythmChangeState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit RhythmChange\n";
}

//damage-------------------------------------------------------------
HitState* HitState::Instance() {
    static HitState inst;
    return &inst;
}
void HitState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter Hit\n";
}
void HitState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void HitState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Hit\n";
}

StunState* StunState::Instance() {
    static StunState inst;
    return &inst;
}
void StunState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter Stun\n";
}
void StunState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void StunState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Stun\n";
}

DeadState* DeadState::Instance() {
    static DeadState inst;
    return &inst;
}
void DeadState::Enter(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Enter Dead\n";
}
void DeadState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void DeadState::Exit(MainPlayerComponent* owner) {
    if(STATE_DEBUG) std::cout << "Exit Dead\n";
}


//attack-------------------------------------------------------------
Attack1State* Attack1State::Instance() {
    static Attack1State inst;
    return &inst;
}
void Attack1State::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0;
    if(STATE_DEBUG) std::cout << "Enter Attack1\n";
}
void Attack1State::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void Attack1State::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Attack1\n";
}

Attack2State* Attack2State::Instance() {
    static Attack2State inst;
    return &inst;
}
void Attack2State::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0;
    if(STATE_DEBUG) std::cout << "Enter Attack2\n";
}
void Attack2State::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void Attack2State::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Attack2\n";
}

Skill1State* Skill1State::Instance() {
    static Skill1State inst;
    return &inst;
}
void Skill1State::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0;
    if(STATE_DEBUG) std::cout << "Enter Skill1\n";
}
void Skill1State::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void Skill1State::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Skill1\n";
}

Skill2State* Skill2State::Instance() {
    static Skill2State inst;
    return &inst;
}
void Skill2State::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0;
    if(STATE_DEBUG) std::cout << "Enter Skill2\n";
}
void Skill2State::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void Skill2State::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Skill2\n";
}

SpecialState* SpecialState::Instance() {
    static SpecialState inst;
    return &inst;
}
void SpecialState::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0;
    if(STATE_DEBUG) std::cout << "Enter Special\n";
}
void SpecialState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void SpecialState::Exit(MainPlayerComponent* owner) {

    if(STATE_DEBUG) std::cout << "Exit Special\n";
}
