#pragma once
#include "System.h"
#include "World.h"

class IntroSequenceComponent;

// PreparePhase 진입 시 시네마틱 카메라 시퀀스를 재생하는 시스템.
// CameraSystem 이후(After)에 실행되어 일반 궤도 카메라/Shake/Dolly 결과를 최종 덮어쓴다.
// 재생 중에는 IntroSequenceComponent::mPlaying 이 켜지며, PlayerInputSystem이 이를 보고 입력을 잠근다.
class IntroSequenceSystem : public System
{
public:
    IntroSequenceSystem(World* w) : System(w) { mPhase = SysPhase::Post; }

    std::vector<std::type_index> After() const override;  // CameraSystem 뒤에서 최종 오버라이드

    void Update(float dt) override;

private:
    // 키프레임을 시간 보간으로 샘플링해 활성 카메라 Transform/FOV에 적용한다.
    void Apply(IntroSequenceComponent* seq, float dt);
    // 재생 종료/해제 — 카메라/입력이 다음 프레임부터 정상 복귀한다.
    void Stop(IntroSequenceComponent* seq);
    // 시퀀스와 뒤따르는 컷인이 모두 끝났으면 서버에 1회 보고한다.
    void TryReportFinished(const IntroSequenceComponent* seq);

private:
    uint8 mPrevPhase = 0;  // 직전 프레임 Phase (Prepare 진입 에지 감지용)

    bool mDoneSeen  = false;  // mDone 을 최소 한 프레임 관측 — 뒤따르는 컷인이 켜질 틈을 준다
    bool mReported  = false;  // 완료 보고 1회 전송 보장
};
