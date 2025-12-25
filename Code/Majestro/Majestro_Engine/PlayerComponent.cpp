#include "pch.h"
#include "json.hpp"
#include <fstream>
#include <limits>
using json = nlohmann::json;
#include "PlayerComponent.h"
#include "StateMachine.h"

//static std::unordered_map<std::string, uint64_t> gFlagByName = {
//    {"F_MOVE", 1ull << 0} ,{"F_STUN", 1ull << 1}, {"F_DEAD", 1ull << 2},{"F_ATTACK", 1ull << 3}, { "F_ANIM", 1ull << 4 }
//};

static StateId NameToId(const std::string& n) {
	if (n == "Idle") return S_Idle;
    if (n == "Walk") return S_Walk;
	if (n == "Run")  return S_Run;
	if (n == "Jump")  return S_Jump;
	if (n == "Dash")  return S_Dash;
	if (n == "Aim")  return S_Aim;
	return 255;
}

MainPlayerComponent::MainPlayerComponent() : mFsm(this), mSpeed(0.0f), mFlags(0ull)
{
}

MainPlayerComponent::MainPlayerComponent(const std::string& path) : mFsm(this), mSpeed(0.0f), mFlags(0ull) 
{
    InitFSMFromJson(path);
};

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


//---------------------------------------------------------------------------------------------------

IdleState* IdleState::Instance() {
    static IdleState inst;
    return &inst;
}
void IdleState::Enter(MainPlayerComponent* owner) {
    ClearFlag(owner->mFlags, FLAG_MOVE);
    std::cout << "Enter Idle\n";
}
void IdleState::Update(MainPlayerComponent* owner) {

}
void IdleState::Exit(MainPlayerComponent* owner) {
    std::cout << "Exit Idle\n";
}


WalkState* WalkState::Instance() {
    static WalkState inst;
    return &inst;
}
void WalkState::Enter(MainPlayerComponent* owner) {
    SetFlag(owner->mFlags, FLAG_MOVE);
    std::cout << "Enter Walk\n";
}
void WalkState::Update(MainPlayerComponent* owner) {
    owner->mFsm.ChangeState(owner, IdleState::Instance());

    if (owner->mFlags & FLAG_NO_RUN) { if (owner->mSpeed > 70.0f) owner->mSpeed = 69.9f; } //달리기 불가 시 속도 강제 다운
    else owner->mFsm.ChangeState(owner, RunState::Instance());
}
void WalkState::Exit(MainPlayerComponent* owner) {
    std::cout << "Exit Walk\n";
}


RunState* RunState::Instance() {                      // [수정] Meyers' singleton (C++11+ 스레드 안전)
    static RunState inst;                          // 최초 호출 시 한 번만 생성
    return &inst;
}
void RunState::Enter(MainPlayerComponent* owner) {
    SetFlag(owner->mFlags, FLAG_MOVE);
    std::cout << "Enter Run\n";
}
void RunState::Update(MainPlayerComponent* owner) {
    if (owner->mFsm.ChangeState(owner, WalkState::Instance())) return;
}
void RunState::Exit(MainPlayerComponent* owner) {
    std::cout << "Exit Run\n";
}


JumpState* JumpState::Instance() {                 
    static JumpState inst;                          
    return &inst;
}
void JumpState::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0.0f;
    owner->mHight = owner->mGround+ 0.1f;
    SetFlag(owner->mFlags, FLAG_JUMP);
    std::cout << "Enter Jump\n";
}
void JumpState::Update(MainPlayerComponent* owner) {
    if (owner->mFsm.ChangeState(owner, IdleState::Instance())) return;
    owner->mHight += owner->mJumpPower * owner->mDt;
    cout << owner->mHight << endl;
}
void JumpState::Exit(MainPlayerComponent* owner) {
    std::cout << "Exit Jump\n";
}


DashState* DashState::Instance() {
    static DashState inst;
    return &inst;
}
void DashState::Enter(MainPlayerComponent* owner) {
    owner->mStateTime = 0.0f;
    std::cout << "Enter Dash\n";
}
void DashState::Update(MainPlayerComponent* owner) {
    if (owner->mStateTime >= this->mStateTime) {
        cout << this->mStateTime << endl;
        owner->mFsm.ChangeState(owner, IdleState::Instance());
        return;
    }
}
void DashState::Exit(MainPlayerComponent* owner) {

    std::cout << "Exit Dash\n";
}