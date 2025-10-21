#include "pch.h"
#include "json.hpp"
#include <fstream>
#include <limits>
using json = nlohmann::json;
#include "PlayerComponent.h"

void MainPlayerComponent::InitFSMOnce() {
	
}

//static std::unordered_map<std::string, uint64_t> gFlagByName = {
//    {"F_MOVE", 1ull << 0} ,{"F_STUN", 1ull << 1}, {"F_DEAD", 1ull << 2},{"F_ATTACK", 1ull << 3}, { "F_ANIM", 1ull << 4 }
//};

static StateId NameToId(const std::string& n) {
	if (n == "Idle") return S_Idle;
    if (n == "Walk") return S_Walk;
	if (n == "Run")  return S_Run;
	return 255;
}

void MainPlayerComponent::InitFSMFromJson(const std::string& path)
{
    cout << "json input" << endl;

    // 1) 포인터→ID 변환기 주입 (상태 이름/ID와 1:1로 일치)
    mFsm.SetIdResolver([](State<MainPlayerComponent>* s)->StateId {
        if (s == IdleState::Instance()) return S_Idle;
        if (s == WalkState::Instance())  return S_Walk;
        if (s == RunState::Instance())  return S_Run;
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