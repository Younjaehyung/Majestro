#pragma once
#include "Component.h"
#include "CameraView.h"

// 씬 진입(PreparePhase 대기) 시 재생되는 시네마틱 카메라 시퀀스 상태.
// 언리얼 시퀀서 export(JSON)를 ResourceManager::LoadCameraSequence로 로드한 키프레임을
// IntroSequenceSystem이 시간축을 따라 재생하며 카메라 Transform/FOV를 덮어쓴다.
// 월드 싱글톤 엔티티에 부착한다(GameRuleComponent와 동일 위치).
//
// [네트워크 안전]
//  - 로컬 전용 카메라 엔티티(NetEntity 아님)의 Transform/FOV만 수정한다.
//  - 플레이어/리플리케이트 엔티티 상태는 일절 건드리지 않는다.
//  - 트리거는 서버 권위 Phase(GameRule.mGamePhase == Prepare)라 모든 클라가 동일 시점에 시작한다.
//  - 재생은 로컬 dt 기반의 순수 비주얼이라 클라 간 미세 편차가 게임플레이/동기화에 영향을 주지 않는다.
class IntroSequenceComponent : public Component<IntroSequenceComponent>
{
public:
    // 로드된 시퀀스 키프레임. 비어 있으면 재생하지 않는다(골격 단계 기본값).
    std::vector<Cinematic::CameraKeyframe> mKeys;

    // 재생 상태
    bool  mPlaying = false;   // 재생 중 — 입력 잠금 / 카메라 오버라이드 게이트
    bool  mDone    = false;   // 1회 재생 완료 — 재진입 방지
    float mElapsed = 0.f;     // 재생 경과 시간(초)

    bool  HasSequence() const { return !mKeys.empty(); }
    float Duration()   const { return mKeys.empty() ? 0.f : mKeys.back().seconds; }
};
