#pragma once

template <class entity_type>
class State
{
public:
    virtual ~State() = default;
    virtual void Enter(entity_type*)=0;   
    virtual void Update(entity_type*)=0;  
    virtual void Exit(entity_type*)=0;    

public:
    bool mAnimOnce = true;
    float mStateTime = 3.0f;
};

using StateId = uint8_t;

struct pair_hash8 {
    size_t operator()(std::pair<StateId, StateId> p) const noexcept {
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
        if (mState) mState->Exit(owner);         
        mState = newState;          
        if (mState) mState->Enter(owner);      
        return true;
    }

    void Update(entity_type* owner) {
        if (mState) mState->Update(owner);       
    }

    using GuardFunc = std::function<bool(entity_type*)>;

    void AddGuardById(StateId from, StateId to, GuardFunc g) {
        transitionGuards[{from, to}] = std::move(g);
    }
    void RemoveGuardById(StateId from, StateId to) {
        transitionGuards.erase({ from, to });
    }

    StateId GetState() { return mIdOf(mState); };

    std::unordered_map<std::pair<StateId, StateId>, GuardFunc, pair_hash8> transitionGuards;

private:
    entity_type* mOwner = nullptr;
    State<entity_type>* mState = nullptr;            
    std::function<StateId(State<entity_type>*)> mIdOf;

    bool CanTransition(entity_type* owner, State<entity_type>* from, State<entity_type>* to) {
        if (!mIdOf) return true;               
        StateId fid = (from ? mIdOf(from) : 255); 
        StateId tid = (to ? mIdOf(to) : 255);

        auto it = transitionGuards.find({ fid, tid });
        if (it == transitionGuards.end()) {
            //std::cout << "[Guard] NOT FOUND  (" << int(fid) << " -> " << int(tid) << ")\n";
            return true;
        }
        //return it->second(owner);
        bool ok = it->second(owner);
        /*std::cout << "[Guard] EVAL (" << int(fid) << " -> " << int(tid)
            << ") = " << ok
            << " | speed=" << owner->mSpeed
            << " | flags=0x" << std::hex << owner->mFlags << std::dec << "\n";*/
        return ok;

    }
};

