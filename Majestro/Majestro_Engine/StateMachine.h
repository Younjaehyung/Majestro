#pragma once

template <class entity_type>
class State
{
public:
    virtual ~State() = default;
    virtual void Enter(entity_type*) = 0;   // 상태 진입 시 호출
    virtual void Update(entity_type*) = 0;  // 매 프레임/루프마다 호출
    virtual void Exit(entity_type*) = 0;    // 상태 종료 시 호출
};

using StateId = uint8_t;

// [추가] (fromId,toId) 키용 경량 해시 (8비트 조합이면 충분)
struct pair_hash8 {
    size_t operator()(std::pair<StateId, StateId> p) const noexcept {
        // 간단 조합: first ^ (second << 8)
        return static_cast<size_t>(p.first) ^ (static_cast<size_t>(p.second) << 8);
    }
};

template <class entity_type>
class StateMachine
{
public:
    explicit StateMachine(entity_type* owner) : mOwner(owner) {}
    void RebindOwner(entity_type* p) { mOwner = p; }

    void SetIdResolver(std::function<StateId(State<entity_type>*)> fn) { mIdOf = std::move(fn); }

    bool ChangeState(entity_type* owner,State<entity_type>* newState) {
        if (newState == mState) return false;
        if (!CanTransition(owner, mState, newState)) { /*cout << "fail" << endl;*/ return false; }
        if (mState) mState->Exit(owner);          // 이전 상태 Exit
        mState = newState;           // 새로운 상태 저장
        if (mState) mState->Enter(owner);         // 새로운 상태 Enter
        return true;
    }

    void Update(entity_type* owner) {
        if (mState) mState->Update(owner);        // 현재 상태 Update
    }

    using GuardFunc = std::function<bool(entity_type*)>;

    void AddGuardById(StateId from, StateId to, GuardFunc g) {
        transitionGuards[{from, to}] = std::move(g);
    }
    void RemoveGuardById(StateId from, StateId to) {
        transitionGuards.erase({ from, to });
    }

    std::unordered_map<std::pair<StateId, StateId>, GuardFunc, pair_hash8> transitionGuards;

private:
    entity_type* mOwner = nullptr;
    State<entity_type>* mState = nullptr;              // 현재 상태
    std::function<StateId(State<entity_type>*)> mIdOf;

    bool CanTransition(entity_type* owner, State<entity_type>* from, State<entity_type>* to) {
        if (!mIdOf) return true;               // [설명] resolver 없으면 가드 스킵(기본 허용)
        StateId fid = (from ? mIdOf(from) : 255);  // [설명] 초기 전이 등 from==nullptr 대비
        StateId tid = (to ? mIdOf(to) : 255);

        auto it = transitionGuards.find({ fid, tid });
        if (it == transitionGuards.end()) {
            //std::cout << "[Guard] NOT FOUND  (" << int(fid) << " -> " << int(tid) << ")\n";
            return true;
        }
        // 등록된 조건 함수 실행
        //return it->second(owner);
        bool ok = it->second(owner);
        /*std::cout << "[Guard] EVAL (" << int(fid) << " -> " << int(tid)
            << ") = " << ok
            << " | speed=" << owner->mSpeed
            << " | flags=0x" << std::hex << owner->mFlags << std::dec << "\n";*/
        return ok;

    }
};



