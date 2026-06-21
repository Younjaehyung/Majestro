#include "pch.h"
#include "IntroSequenceSystem.h"
#include "IntroSequenceComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"        // MainCameraComponent
#include "TransformComponent.h"
#include "CameraSystem.h"
#include "GameRuleComponent.h"

std::vector<std::type_index> IntroSequenceSystem::After() const
{
    return { typeid(CameraSystem) };
}

void IntroSequenceSystem::Update(float dt)
{
    if (!mWorld->HasComponentPool<IntroSequenceComponent>())
        return;

    const Entity singleton = mWorld->GetSingletonEntity();
    IntroSequenceComponent* seq = mWorld->GetComponent<IntroSequenceComponent>(singleton);
    if (!seq)
        return;

    // ---- 현재 Phase 조회 (서버 권위) ----
    uint8 curPhase = mPrevPhase;
    if (GameRuleComponent* rule = mWorld->GetComponent<GameRuleComponent>(singleton))
        curPhase = rule->mGamePhase;

    // ---- 트리거: PreparePhase 진입 에지에서 1회 재생 시작 ----
    const uint8 prepare = static_cast<uint8>(WavePhaseType::Prepare);
    const bool enteredPrepare = (curPhase == prepare) && (mPrevPhase != prepare);
    mPrevPhase = curPhase;

    if (enteredPrepare && !seq->mDone && seq->HasSequence())
    {
        seq->mPlaying = true;
        seq->mElapsed = 0.f;
    }

    if (!seq->mPlaying)
        return;

    // ---- Prepare 단계를 벗어나면 즉시 종료(카메라/입력 즉시 복귀) ----
    if (curPhase != prepare)
    {
        Stop(seq);
        return;
    }

    Apply(seq, dt);

    // 마지막 키프레임 도달 시 종료
    if (seq->mElapsed >= seq->Duration())
        Stop(seq);
}

void IntroSequenceSystem::Apply(IntroSequenceComponent* seq, float dt)
{
    seq->mElapsed += dt;

    // 활성 카메라 엔티티(로컬 전용) 조회
    auto cams = mWorld->GetEntitiesWithComponents<MainCameraComponent, CameraComponent, TransformComponent>();
    if (cams.empty())
        return;

    const Entity camE = cams.front();
    CameraComponent*    cam = mWorld->GetComponent<CameraComponent>(camE);
    TransformComponent* tr  = mWorld->GetComponent<TransformComponent>(camE);
    if (!cam || !tr)
        return;

    const auto& keys = seq->mKeys;
    const float t = seq->mElapsed;

    // 키프레임 구간 보간 (작은 시퀀스라 선형 탐색으로 충분)
    CameraView view;
    if (t <= keys.front().seconds)
    {
        view = keys.front().view;
    }
    else if (t >= keys.back().seconds)
    {
        view = keys.back().view;
    }
    else
    {
        size_t i = 0;
        while (i + 1 < keys.size() && keys[i + 1].seconds <= t)
            ++i;

        const CameraKeyframe& a = keys[i];
        const CameraKeyframe& b = keys[i + 1];
        const float span = b.seconds - a.seconds;
        const float u = (span > 1e-5f) ? (t - a.seconds) / span : 0.f;

        view.position = Vec3::Lerp(a.view.position, b.view.position, u);
        view.rotation = Quaternion::Slerp(a.view.rotation, b.view.rotation, u);
        view.fovDeg   = a.view.fovDeg + (b.view.fovDeg - a.view.fovDeg) * u;
    }

    // Transform 적용
    tr->mLocalPosition = view.position;
    const Vec3 e = view.rotation.ToEuler();
    tr->mLocalRotationE = Vec3(XMConvertToDegrees(e.x),
                               XMConvertToDegrees(e.y),
                               XMConvertToDegrees(e.z));

    // FOV: UE 가로 화각(도) → 종횡비 기반 세로 화각(라디안) (MainMenuCameraSystem와 동일 변환)
    const float aspect  = cam->mWidth / cam->mHeight;
    const float hFovRad = XMConvertToRadians(view.fovDeg);
    const float vFovRad = 2.f * atanf(tanf(hFovRad * 0.5f) / aspect);
    cam->SetFOV(vFovRad);

    // CameraSystem이 이미 뷰/투영 행렬을 만든 뒤(After)이므로,
    // 우리가 덮어쓴 Transform/FOV로 월드 행렬과 뷰·투영 행렬을 다시 빌드한다.
    // (CameraSystem.cpp의 transform->FinalUpdate() + camera->FinalUpdate(...) 와 동일)
    tr->FinalUpdate();
    cam->FinalUpdate(tr->GetWorldMatrix().Invert());
}

void IntroSequenceSystem::Stop(IntroSequenceComponent* seq)
{
    seq->mPlaying = false;
    seq->mDone    = true;
}
