#pragma once
#include "EnginePch.h"

/*
 * AnimationGraph
 * ─────────────────────────────────────────────────────────────────────────────
 * 역할: 상태 전환별로 블렌딩 시간(duration)과 곡선(curve)을 데이터로 관리한다.
 *
 * 서버는 클립 인덱스(stateId / lowerStateId)를 패킷으로 보내고,
 * 클라이언트의 AnimationSystem은 AnimationGraph를 조회하여 전환마다 적절한
 * 블렌딩 파라미터를 결정한다.
 *
 * 구성:
 *   AnimBlendCurve   — 보간 커브 종류
 *   AnimGraphTransition — 단일 전환 규칙 (from → to, duration, curve)
 *   AnimGraphLayer   — Lower/Upper 레이어별 전환 규칙 묶음
 *   AnimationGraph   — 전체 그래프 (JSON 로드, 전환 조회 API)
 *
 * JSON 포맷 예시 (Resources/Json/AnimGraph_Player.json):
 * {
 *   "defaultLowerDuration": 0.2,
 *   "defaultUpperDuration": 0.15,
 *   "lower": [
 *     { "from": 0, "to": 1, "duration": 0.2,  "curve": "EaseOut" },
 *     { "from": 1, "to": 2, "duration": 0.15, "curve": "Linear"  }
 *   ],
 *   "upper": [
 *     { "from": "*", "to": 3, "duration": 0.05, "curve": "Linear" }
 *   ]
 * }
 * "from": "*" 는 어떤 상태에서 전환해도 적용되는 와일드카드 규칙이다.
 */

// ─────────────────────────────────────────────────────────────────────────────
// 보간 커브 종류
// ─────────────────────────────────────────────────────────────────────────────
enum class AnimBlendCurve : uint8
{
    Linear    = 0,  // 선형 보간
    EaseIn    = 1,  // 천천히 시작, 빠르게 끝
    EaseOut   = 2,  // 빠르게 시작, 천천히 끝
    EaseInOut = 3,  // 부드럽게 시작·끝
};

// ─────────────────────────────────────────────────────────────────────────────
// 단일 전환 규칙
// ─────────────────────────────────────────────────────────────────────────────
struct AnimGraphTransition
{
    static constexpr uint32 WILDCARD = 0xFFFF'FFFFu; // "from": "*"

    uint32          fromClip  = WILDCARD;   // 0xFFFF'FFFF 이면 와일드카드
    uint32          toClip    = WILDCARD;
    float           duration  = 0.2f;       // 블렌딩 시간 (초)
    AnimBlendCurve  curve     = AnimBlendCurve::Linear;
};

// ─────────────────────────────────────────────────────────────────────────────
// 레이어별 전환 규칙 묶음 (Lower 또는 Upper)
// ─────────────────────────────────────────────────────────────────────────────
struct AnimGraphLayer
{
    float                           defaultDuration = 0.2f;     // 규칙이 없을 때 기본값
    vector<AnimGraphTransition>     transitions;                 // 전환 규칙 목록

    // fromClip → toClip 에 해당하는 duration 반환.
    // 매칭 우선순위: 정확 일치 > from 와일드카드 > defaultDuration
    float GetDuration(uint32 from, uint32 to) const;

    // 보간 커브 반환 (동일 우선순위)
    AnimBlendCurve GetCurve(uint32 from, uint32 to) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// 전체 AnimationGraph
// ─────────────────────────────────────────────────────────────────────────────
class AnimationGraph
{
public:
    AnimationGraph() = default;

    // JSON 파일에서 그래프 로드
    // 실패 시 false 반환 (기본값으로 동작)
    bool LoadFromJson(const wstring& path);

    // Lower 레이어 전환 duration / curve 조회
    float          GetLowerDuration(uint32 from, uint32 to) const;
    AnimBlendCurve GetLowerCurve   (uint32 from, uint32 to) const;

    // Upper 레이어 전환 duration / curve 조회
    float          GetUpperDuration(uint32 from, uint32 to) const;
    AnimBlendCurve GetUpperCurve   (uint32 from, uint32 to) const;

    // Upper 레이어 Enter/Exit 전환 시간 조회
    float GetUpperEnterDuration() const { return mUpperEnterDuration; }
    float GetUpperExitDuration()  const { return mUpperExitDuration; }

private:
    AnimGraphLayer  mLower;
    AnimGraphLayer  mUpper;

    float           mUpperEnterDuration = 0.15f;    // Upper 레이어 켜질 때 보간 시간
    float           mUpperExitDuration  = 0.1f;     // Upper 레이어 꺼질 때 보간 시간
};
