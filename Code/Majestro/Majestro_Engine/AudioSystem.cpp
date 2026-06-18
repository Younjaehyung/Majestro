#include "pch.h"
#include "Engine.h"
#include "Timer.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "BeatSystem.h"
#include "PlayerComponent.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "TagComponent.h"
#include "GameRuleComponent.h"
#include "PlayerStatusComponent.h"

namespace
{
    constexpr float kRhythmCompareEpsilon = 0.001f;

    void SendRhythmChangedPacket(World* world, Entity playerEntity, uint8 prevRhythm, uint8 changedRhythm, uint8 playerType)
    {
        NetEntityComponent* netEntityComponent = world->GetComponent<NetEntityComponent>(playerEntity);
        if (netEntityComponent == nullptr)
            return;

        C2S_RhythmChangedPacket pkt{};
        pkt.netEntityId = netEntityComponent->mNetEntityId;
        pkt.previousRhythm = prevRhythm;
        pkt.changedRhythm = changedRhythm;
        pkt.playerType = playerType;

        SendRequest req{};
        req.Type = PKT_Type::C2S_PKT_RHYTHM_CHANGED;
        req.SIze = sizeof(C2S_RhythmChangedPacket);
        req.StoreAs(pkt);
        gSendBuffer.Push(req);
    }
}

AudioSystem::AudioSystem(World* world) : System::System(world)
{
}

void AudioSystem::Initialize()
{
   
    AUDIOMANAGER.PreloadBanks({"MajestroBank.bank"});

    // 리듬 음악
    const SceneId sceneId = mWorld->GetSceneId();
    const bool inGame = (sceneId == SceneId::FirstGame || sceneId == SceneId::SecondGame || sceneId == SceneId::ThirdGame);

    if (inGame)
    {
        AUDIOMANAGER.RequestBGM("event:/OST/ElecMulti", SOUNDNAME::Elec);
        AUDIOMANAGER.RequestBGM("event:/OST/BassMulti", SOUNDNAME::Bass);
        AUDIOMANAGER.RequestBGM("event:/OST/DrumMulti", SOUNDNAME::Drum);

        // T0 정렬 전까지 무음(일시정지)
        AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Elec, true);
        AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Bass, true);
        AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Drum, true);
    }
    else
    {
        AUDIOMANAGER.StopBGM(SOUNDNAME::Elec);
        AUDIOMANAGER.StopBGM(SOUNDNAME::Bass);
        AUDIOMANAGER.StopBGM(SOUNDNAME::Drum);
    }
    

}

void AudioSystem::Update(float deltaTime)
{

    FMOD_3D_ATTRIBUTES listener{};
    listener.position = { 0, 0, 0 };
    listener.forward = { 0, 0, 1 };
    listener.up = { 0, 1, 0 };

    if (mWorld->HasComponentPool<MainCameraComponent>())
    {
        auto cameraEntities = mWorld->GetEntitiesWithComponent<MainCameraComponent>();
        if (!cameraEntities.empty())
        {
            if (TransformComponent* cameraTransform = mWorld->GetComponent<TransformComponent>(cameraEntities[0]))
            {
                const Matrix& world = cameraTransform->mWorldMatrix;
                listener.position = { world._41, world._42, world._43 };

                Vec3 forward(world._31, world._32, world._33);
                Vec3 up(world._21, world._22, world._23);
                forward.Normalize();
                up.Normalize();
                listener.forward = { forward.x, forward.y, forward.z };
                listener.up = { up.x, up.y, up.z };
            }
        }
    }
    AUDIOMANAGER.SetListener(listener);

    // FMOD 시킹 검증(디버그)
    if (INPUT.GetKeyDown(eKeyCode::NUMPAD1))    // 2초 위치 시킹
        AUDIOMANAGER.DebugStartSeekProbe(2.0f); 
    if (INPUT.GetKeyDown(eKeyCode::NUMPAD2))    // 20초 위치 시킹
        AUDIOMANAGER.DebugStartSeekProbe(20.0f);

    // 곡 시작 T0 정렬
    AlignBgmToServerSongClock();

    // 드리프트 보정
    CorrectBgmDrift();

    UpdateSilenceMusicState();

    time += deltaTime;

    // 호위 BGM 파라미터
    if (mWorld->HasComponentPool<GameEscortComponent>())
    {
        auto escortEntities = mWorld->GetEntitiesWithComponent<GameEscortComponent>();
        if (!escortEntities.empty())
        {
            GameEscortComponent* escort = mWorld->GetComponent<GameEscortComponent>(escortEntities[0]);
            if (escort != nullptr && static_cast<int>(escort->mEscortStage) != mPrevEscortStage)
            {
                mPrevEscortStage = static_cast<int>(escort->mEscortStage);


                float param = 0.5f * static_cast<float>(mPrevEscortStage);
                if (param > 1.f) param = 1.f;
                AUDIOMANAGER.SetBGMParam("NextEscort", SOUNDNAME::Ambient, param, true);
                std::cout << "[BGM] NextEscort=" << param << " (escortStage=" << mPrevEscortStage << ")" << std::endl;
            }
        }
    }

    if (!mWorld->HasComponentPool<MainPlayerComponent>())
        return;

    auto playerEntities = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();
    if (playerEntities.empty())
        return;

    // 공유 Song Clock 절대 박자
    int64 currentBeat = -1;
    if (auto systemManager = mWorld->GetSystemManager())
    {
        if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
            currentBeat = beatSystem->GetAbsoluteBeatIndex();
    }

    for (Entity playerEntity : playerEntities)
    {
        MainPlayerComponent* playerComponent = mWorld->GetComponent<MainPlayerComponent>(playerEntity);
        if (playerComponent == nullptr)
            continue;

        LocalPlayerComponent* localPlayer = mWorld->GetComponent<LocalPlayerComponent>(playerEntity);

        // 리듬 변경 전송
        if (localPlayer && playerComponent->mRhythmSettleTimer > 0.f)
        {
            playerComponent->mRhythmSettleTimer -= deltaTime;
            if (playerComponent->mRhythmSettleTimer <= 0.f)
            {
                playerComponent->mRhythmSettleTimer = 0.f;
                
                const bool airborneForRhythmChange =
                    playerComponent->mLowerState == static_cast<int>(ReplicatedMovementMode::Airborne) ||
                    playerComponent->mLowerState == static_cast<int>(ReplicatedMovementMode::Falling) ||
                    playerComponent->mLowerState == static_cast<int>(ReplicatedMovementMode::Landing);

                if (!airborneForRhythmChange &&
                    !playerComponent->mRhythmChangeInFlight &&
                    playerComponent->mDesiredRhythm != playerComponent->mRhythm)
                {
                    SendRhythmChangedPacket(mWorld, playerEntity, playerComponent->mRhythm, playerComponent->mDesiredRhythm, playerComponent->mPlayerType);
                    playerComponent->mRhythmChangeInFlight = true;   // 적용될 때까지 추가 송신 차단
                }
            }
        }

        // 전환 적용: 서버가 지정한 절대 박자에 도달했을 때만 오디오/리듬 상태를 바꾼다.
        if (playerComponent->mHasQueuedRhythmChange &&
            currentBeat >= playerComponent->mRhythmApplyBeat)
        {
            ApplyRhythmLayerByPlayerType(playerComponent->mPlayerType, playerComponent->mNextRhythm);

            // 디버그 : 전환이 곡의 어느 위치에서 일어나는지. 
            int fmodMs = 0;

            AUDIOMANAGER.GetBGMTimelinePositionMs(SOUNDNAME::Elec, fmodMs);


            std::cout << "[RhythmApply] curBeat=" << currentBeat
                      << " beatInBar=" << (currentBeat % 16)
                      << " fmodPos=" << (fmodMs / 1000.f) << "s"
                      << " -> rhythm=" << (int)playerComponent->mNextRhythm << std::endl;
            // 마디 정렬이 맞으면 beatInBar = 0, fmodPos = 0(또는 mLoopLen 배수 근처)


            playerComponent->mRhythm = playerComponent->mNextRhythm;
            playerComponent->mHasQueuedRhythmChange = false;
            playerComponent->mRhythmApplyBeat = -1;
            playerComponent->mRhythmChangeInFlight = false;      // 전환 완료 (다음 요청 허용)

            
            // 최신 desired 로 한 번에 전환
            if (localPlayer && playerComponent->mDesiredRhythm != playerComponent->mRhythm)
                playerComponent->mRhythmSettleTimer = MainPlayerComponent::kRhythmSettleTime;
        }
    }

}


// 곡 시작 T0 정렬 
void AudioSystem::AlignBgmToServerSongClock()
{
    if (mBgmStartAligned)
        return;

    // 곡 인스턴스가 생성 성공 여부 확인
    int probeMs = 0;
    if (!AUDIOMANAGER.GetBGMTimelinePositionMs(SOUNDNAME::Elec, probeMs))
        return;

    auto systemManager = mWorld->GetSystemManager();
    if (systemManager == nullptr)
        return;

    BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>();
    if (beatSystem == nullptr)
        return;

    // 서버 sync 가 한 번이라도 도착한 뒤에 시작
    if (!beatSystem->HasSynced())
        return;

    // 일시정지 상태에서 박자 위치로 시킹 요청
    if (mAlignSeekFrame < 0)
    {
        float targetPhase = fmodf(beatSystem->GetSongPosition(), mLoopLen);
        if (targetPhase < 0.f)
            targetPhase += mLoopLen;

        AUDIOMANAGER.SeekBGM(SOUNDNAME::Drum, targetPhase);
        AUDIOMANAGER.SeekBGM(SOUNDNAME::Bass, targetPhase);
        AUDIOMANAGER.SeekBGM(SOUNDNAME::Elec, targetPhase);

        mAlignSeekFrame = 0;
        std::cout << "[T0] seek to beat phase = " << targetPhase << "s (paused)" << std::endl;
        return;
    }

    // 시킹이 비동기로 반영될 시간 대기 후 정렬된 위치에서 재생 시작.
    if (++mAlignSeekFrame < 5)
        return;

    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Drum, false);
    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Bass, false);
    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Elec, false);

    mBgmStartAligned = true;
    std::cout << "[T0] BGM aligned & resumed" << std::endl;
}

// 드리프트 보정(피치 너지)
void AudioSystem::CorrectBgmDrift()
{
    if (!mBgmStartAligned)   // T0 정렬 이후에만
        return;

    auto systemManager = mWorld->GetSystemManager();
    if (systemManager == nullptr)
        return;

    BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>();
    if (beatSystem == nullptr || !beatSystem->HasSynced())
        return;

    // 다 똑같아서 Elec으로 일단 하는중임
    int fmodMs = 0;
    if (!AUDIOMANAGER.GetBGMTimelinePositionMs(SOUNDNAME::Elec, fmodMs))
        return;

    // 위상 비교
    const float fmodPhase = fmodf(fmodMs / 1000.f, mLoopLen);
    const float beatPhase = fmodf(beatSystem->GetSongPosition(), mLoopLen);
    float driftRaw = fmodPhase - beatPhase;
    // 최단 방향으로 보정(-loopLen/2 ~ +loopLen/2)
    if (driftRaw >  mLoopLen * 0.5f) driftRaw -= mLoopLen;
    if (driftRaw < -mLoopLen * 0.5f) driftRaw += mLoopLen;

    // EMA 평활화 (부드럽게 보정)
    mDriftSmoothed = mDriftSmoothed * (1.0f - mDriftEmaAlpha) + driftRaw * mDriftEmaAlpha;


    // drift>0(클라 음악이 서버보다 앞섬) : 느리게(pitch<1)
    // drift<0(뒤처짐) : 빠르게(pitch>1)

    float pitch = 1.0f;
    if (fabsf(mDriftSmoothed) > mDriftDeadzone)
    {
        float nudge = -mDriftSmoothed * mDriftGain; // 앞서면 음수(느리게), 뒤처지면 양수(빠르게)
        if (nudge >  mDriftMaxNudge) nudge =  mDriftMaxNudge;
        if (nudge < -mDriftMaxNudge) nudge = -mDriftMaxNudge;
        pitch = 1.0f + nudge;
    }

    AUDIOMANAGER.SetBGMPitch(SOUNDNAME::Drum, pitch);
    AUDIOMANAGER.SetBGMPitch(SOUNDNAME::Bass, pitch);
    AUDIOMANAGER.SetBGMPitch(SOUNDNAME::Elec, pitch);

    // 검증 로그(1초마다):   
    mDriftLogTimer += DELTA_TIME;
    if (mDriftLogTimer >= 1.0f)
    {
        mDriftLogTimer = 0.f;
        std::cout << "[Drift] raw=" << (driftRaw * 1000.f)          // 드리프트 raw
                  << "ms smooth=" << (mDriftSmoothed * 1000.f)      // 평활(ms)
                  << "ms pitch=" << pitch << std::endl;             // 적용 피치
    }
}

void AudioSystem::ApplyRhythmLayerByPlayerType(uint8 playerType, uint8 rhythm)
{
    if (rhythm >= static_cast<uint8>(Rhythm::Count))
        return;

    SOUNDNAME stem;
    const char* paramName;
    switch (playerType)
    {
    case 0: stem = SOUNDNAME::Drum; paramName = "DrumParam"; break; // Drum player — DrumMulti
    case 1: stem = SOUNDNAME::Bass; paramName = "BassParam"; break; // Bass player — BassMulti
    case 2: stem = SOUNDNAME::Elec; paramName = "ElecParam"; break; // Elec player — ElecMulti
    default: return;
    }

    AUDIOMANAGER.SetBGMParam(paramName, stem, static_cast<float>(rhythm), true);
}

void AudioSystem::UpdateSilenceMusicState()
{
    Entity localPlayerEntity = NULL_ENTITY;

    if (mWorld->HasComponentPool<MainPlayerComponent>() &&
        mWorld->HasComponentPool<LocalPlayerComponent>())
    {
        const auto localPlayers =
            mWorld->GetEntitiesWithComponents<MainPlayerComponent, LocalPlayerComponent>();

        if (!localPlayers.empty())
            localPlayerEntity = localPlayers.front();
    }

    if (localPlayerEntity == NULL_ENTITY)
    {
        if (mSilenceMusicMuted && mSilenceMusicStem != SOUNDNAME::End)
            AUDIOMANAGER.SetBGMVolume(mSilenceMusicStem, 1.0f);

        mSilenceMusicMuted = false;
        mSilenceMusicStem = SOUNDNAME::End;
        return;
    }

    const MainPlayerComponent* player =
        mWorld->GetComponent<MainPlayerComponent>(localPlayerEntity);
    if (player == nullptr)
        return;

    SOUNDNAME playerMusic = SOUNDNAME::End;
    switch (player->mPlayerType)
    {
    case PlayerType::Rudwig:
        playerMusic = SOUNDNAME::Drum;
        break;
    case PlayerType::Ibanix:
        playerMusic = SOUNDNAME::Bass;
        break;
    case PlayerType::Fanthor:
        playerMusic = SOUNDNAME::Elec;
        break;
    default:
        return;
    }

    if (mSilenceMusicStem != playerMusic)
    {
        if (mSilenceMusicMuted && mSilenceMusicStem != SOUNDNAME::End)
            AUDIOMANAGER.SetBGMVolume(mSilenceMusicStem, 1.0f);

        mSilenceMusicStem = playerMusic;
        mSilenceMusicMuted = false;
    }

    const PlayerStatusComponent* status =
        mWorld->GetComponent<PlayerStatusComponent>(localPlayerEntity);
    const bool shouldMute =
        status != nullptr &&
        status->FindBuff(ReplicatedBuffType::Silence) != nullptr;

    if (mSilenceMusicMuted == shouldMute)
        return;

    // 수정사항: 재생 위치를 멈추지 않고 로컬 캐릭터에 대응하는 음악만 음소거한다.
    // 개별 배율은 Ambient 버스 음량과 곱해지므로 설정 음량 비율이 그대로 유지된다.
    AUDIOMANAGER.SetBGMVolume(playerMusic, shouldMute ? 0.0f : 1.0f);
    mSilenceMusicMuted = shouldMute;
}

void OnExplosion(float x, float y, float z) {
    FMOD_3D_ATTRIBUTES a{};
    a.position = { x, y, z };
    a.forward = { 0, 0, 1 };
    a.up = { 0, 1, 0 };
    AUDIOMANAGER.PlayOneShot3D("event:/Drum", a);
}

void AudioSystem::Shutdown()
{
    AUDIOMANAGER.Shutdown();
}
