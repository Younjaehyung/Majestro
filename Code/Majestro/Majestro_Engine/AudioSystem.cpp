#include "pch.h"
#include "Engine.h"
#include "MajestroGameInstance.h"
#include "Timer.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "BeatSystem.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "TagComponent.h"
#include "GameRuleComponent.h"
#include "PlayerStatusComponent.h"
#include "EngineLog.h"

namespace
{
    constexpr float kRhythmCompareEpsilon = 0.001f;

    // setTimelinePosition 이 FMOD 에 반영되기까지 기다리는 프레임 수.
    constexpr int kBgmAlignSeekFrames = 5;

    // 보스 음악 하드 시킹 기준
    constexpr float kBossResyncSeconds = 0.06f;

    // 하드 시킹이 반영될 때까지 재판정을 미루는 시간(초).
    constexpr float kBossResyncCooldown = 0.5f;

    struct RhythmStemConfig
    {
        PlayerType playerType;
        SOUNDNAME stem;
        const char* eventPath;
        const char* parentParam;
        const char* subParam;
    };

    constexpr std::array<RhythmStemConfig, static_cast<size_t>(PlayerType::Count)>
        kRhythmStemConfigs = {
        RhythmStemConfig{ PlayerType::Rudwig, SOUNDNAME::Drum,
            "event:/OST/DrumMulti", "DrumParam", "DrumSubParam" },
        RhythmStemConfig{ PlayerType::Ibanix, SOUNDNAME::Bass,
            "event:/OST/BassMulti", "BassParam", "BassSubParam" },
        RhythmStemConfig{ PlayerType::Fanthor, SOUNDNAME::Elec,
            "event:/OST/ElecMulti", "ElecParam", "ElecSubParam" }
    };

    static_assert(
        kRhythmStemConfigs.size() == static_cast<size_t>(PlayerType::Count),
        "Rhythm stem config must cover every player type.");


    constexpr std::array<BossMusicConfig, kBossMusicSlotCount> kBossMusicConfigs = {
        BossMusicConfig{ static_cast<uint8>(EnemyType::Brass),  SOUNDNAME::BrassBoss,
            "event:/OST/BrassBossMulti",  "StringParam" },
        BossMusicConfig{ static_cast<uint8>(EnemyType::Dragon), SOUNDNAME::StringBoss,
            "event:/OST/StringBossMulti", "StringParam" }
    };

    const RhythmStemConfig* FindRhythmStemConfig(PlayerType playerType)
    {
        for (const RhythmStemConfig& config : kRhythmStemConfigs)
        {
            if (config.playerType == playerType)
                return &config;
        }

        return nullptr;
    }

    void SendRhythmChangedPacket(
        World* world,
        Entity playerEntity,
        Rhythm previousRhythm,
        Rhythm changedRhythm,
        PlayerType playerType)
    {
        NetEntityComponent* netEntityComponent = world->GetComponent<NetEntityComponent>(playerEntity);
        if (netEntityComponent == nullptr)
            return;

        C2S_RhythmChangedPacket pkt{};
        pkt.netEntityId = netEntityComponent->mNetEntityId;
        pkt.previousRhythm = ToRhythmValue(previousRhythm);
        pkt.changedRhythm = ToRhythmValue(changedRhythm);
        pkt.playerType = static_cast<uint8>(playerType);

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
    mBgmStartAligned = false;
    mBgmInitializationFailed = false;
    mAlignSeekFrame = -1;
   
    AUDIOMANAGER.PreloadBanks({"MajestroBank.bank"});

    // 리듬 음악
    const SceneId sceneId = mWorld->GetSceneId();
    const bool inGame = IsLevelScene(sceneId);

    if (inGame)
    {
        bool allRhythmStemsPrepared = true;

        for (const RhythmStemConfig& config : kRhythmStemConfigs)
        {
         const bool prepared = AUDIOMANAGER.RequestBGM(
                config.eventPath,
                config.stem,
                { { config.subParam, 0.0f }, { config.parentParam, 0.0f } },
                true);
            allRhythmStemsPrepared = allRhythmStemsPrepared && prepared;
        }

        if (!allRhythmStemsPrepared)
        {

            for (const RhythmStemConfig& config : kRhythmStemConfigs)
                AUDIOMANAGER.StopBGM(config.stem);

            mBgmInitializationFailed = true;
            EngineLog::WriteOnce(
                EngineLog::Domain::AudioDiagnostic,
                "rhythm-stem-initialization",
                "[BGM] rhythm stem initialization failed");
        }

      
        StopAllBossMusic();
    }
    else
    {
        AUDIOMANAGER.StopBGM(SOUNDNAME::Elec);
        AUDIOMANAGER.StopBGM(SOUNDNAME::Bass);
        AUDIOMANAGER.StopBGM(SOUNDNAME::Drum);
        StopAllBossMusic();
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

    // 오디오 런타임 추적이 켜진 경우에만 시킹 검증 입력도 확인한다.
    if (EngineLog::Enabled(EngineLog::Domain::AudioRuntime))
    {
        if (INPUT.GetKeyDown(eKeyCode::NUMPAD1))
            AUDIOMANAGER.DebugStartSeekProbe(2.0f);
        if (INPUT.GetKeyDown(eKeyCode::NUMPAD2))
            AUDIOMANAGER.DebugStartSeekProbe(20.0f);
    }

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

                // EscortParam: 0 = Prepare 재생, 1 = 첫 중간거점까지, 2 = 중간거점 점령 후 재생
                float param = static_cast<float>(mPrevEscortStage);
                if (param > 2.f) param = 2.f;
                AUDIOMANAGER.SetBGMParam("EscortParam", SOUNDNAME::Ambient, param, true);
                EngineLog::Write(
                    EngineLog::Domain::AudioRuntime,
                    "[BGM] EscortParam=",
                    param,
                    " escortStage=",
                    mPrevEscortStage);
            }
        }
    }

    // 보스 전용 음악
    UpdateBossMusic();

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

                const uint8 serverTarget = playerComponent->mHasQueuedRhythmChange
                    ? playerComponent->mNextRhythm
                    : playerComponent->mRhythm;
                const bool reservationNeedsUpdate =
                    playerComponent->mDesiredRhythm != serverTarget;

                if (!playerComponent->mRhythmChangeInFlight &&
                    reservationNeedsUpdate)
                {
                    if (airborneForRhythmChange)
                    {
                        // 공중 상태가 끝난 뒤 최신 예약을 다시 전송하도록 대기
                        playerComponent->mRhythmSettleTimer =
                            MainPlayerComponent::kRhythmSettleTime;
                    }
                    else
                    {
                        SendRhythmChangedPacket(
                            mWorld,
                            playerEntity,
                            SanitizeRhythm(playerComponent->mRhythm),
                            SanitizeRhythm(playerComponent->mDesiredRhythm),
                            playerComponent->mPlayerType);
                        // 응답을 받기 전까지만 추가 요청을 막고 이후에는 예약 교체를 허용
                        playerComponent->mRhythmChangeInFlight = true;
                    }
                }
            }
        }

        // 서버가 지정한 박자에서 현재 리듬과 FMOD 음악을 함께 적용
        if (playerComponent->IsPendingRhythmReady(currentBeat))
        {
            const Rhythm appliedRhythm = SanitizeRhythm(playerComponent->mNextRhythm);

            ApplyRhythmToStem(
                playerComponent->mPlayerType,
                appliedRhythm);

            // 서버가 지정한 박자에서 현재 리듬과 음악을 함께 확정
            playerComponent->ApplyPendingRhythmChange();

            // 로그가 꺼져 있으면 진단용 FMOD 조회 자체를 수행하지 않는다.
            if (EngineLog::Enabled(EngineLog::Domain::AudioRuntime))
            {
                int fmodMs = 0;
                AUDIOMANAGER.GetBGMTimelinePositionMs(SOUNDNAME::Elec, fmodMs);
                EngineLog::Write(
                    EngineLog::Domain::AudioRuntime,
                    "[RhythmApply] curBeat=",
                    currentBeat,
                    " beatInLoop=",
                    currentBeat % kMusicLoopBeatCount,
                    " fmodPos=",
                    fmodMs / 1000.f,
                    "s rhythm=",
                    static_cast<int>(ToRhythmValue(appliedRhythm)));
            }


            if (localPlayer && playerComponent->mDesiredRhythm != playerComponent->mRhythm)
                playerComponent->mRhythmSettleTimer = MainPlayerComponent::kRhythmSettleTime;
        }
    }

}


// 곡 시작 T0 정렬 
void AudioSystem::AlignBgmToServerSongClock()
{
    if (mBgmStartAligned || mBgmInitializationFailed)
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
        const float targetPhase = WrapToLoop(beatSystem->GetSongPosition());

        AUDIOMANAGER.SeekBGM(SOUNDNAME::Drum, targetPhase);
        AUDIOMANAGER.SeekBGM(SOUNDNAME::Bass, targetPhase);
        AUDIOMANAGER.SeekBGM(SOUNDNAME::Elec, targetPhase);

        mAlignSeekFrame = 0;
        EngineLog::Write(
            EngineLog::Domain::AudioRuntime,
            "[T0] seek to beat phase=",
            targetPhase,
            "s paused");
        return;
    }

    // 시킹이 비동기로 반영될 시간 대기 후 정렬된 위치에서 재생 시작.
    if (++mAlignSeekFrame < kBgmAlignSeekFrames)
        return;

    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Drum, false);
    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Bass, false);
    AUDIOMANAGER.SetBGMPaused(SOUNDNAME::Elec, false);

    mBgmStartAligned = true;
    EngineLog::Write(
        EngineLog::Domain::AudioRuntime,
        "[T0] BGM aligned and resumed");
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
    const float fmodPhase = WrapToLoop(fmodMs / 1000.f);
    const float beatPhase = WrapToLoop(beatSystem->GetSongPosition());
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

    // 보스 음악 피치 적용
    CorrectBossMusicDrift(fmodPhase, pitch);

    // 런타임 오디오 추적이 활성화된 경우에만 로그 타이머도 갱신한다.
    if (EngineLog::Enabled(EngineLog::Domain::AudioRuntime))
    {
        mDriftLogTimer += DELTA_TIME;
        if (mDriftLogTimer >= 1.0f)
        {
            mDriftLogTimer = 0.f;
            EngineLog::Write(
                EngineLog::Domain::AudioRuntime,
                "[Drift] raw=",
                driftRaw * 1000.f,
                "ms smooth=",
                mDriftSmoothed * 1000.f,
                "ms pitch=",
                pitch);
        }
    }
}

void AudioSystem::ApplyRhythmToStem(PlayerType playerType, Rhythm rhythm)
{
    const RhythmStemConfig* config = FindRhythmStemConfig(playerType);
    if (config == nullptr)
        return;

    const RhythmVariantSelection& selection = MajestroGameInstance::GetInstance()
            .GetConfirmedRhythmVariantSelectionForPlayerType(static_cast<uint8>(playerType));
    const RhythmSubVariant subVariant = selection.GetForRhythm(rhythm);

    // 리듬 변경
    AUDIOMANAGER.SetBGMParam(
        config->subParam,
        config->stem,
        static_cast<float>(static_cast<uint8>(subVariant)),
        true);
    AUDIOMANAGER.SetBGMParam(
        config->parentParam,
        config->stem,
        static_cast<float>(ToRhythmValue(rhythm)),
        true);
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

    const RhythmStemConfig* config = FindRhythmStemConfig(player->mPlayerType);
    if (config == nullptr)
        return;

    const SOUNDNAME playerMusic = config->stem;

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


    AUDIOMANAGER.SetBGMVolume(playerMusic, shouldMute ? 0.0f : 1.0f);
    mSilenceMusicMuted = shouldMute;
}

void AudioSystem::UpdateBossMusic()
{
    const bool hasEnemies = mWorld->HasComponentPool<EnemyComponent>();

    for (size_t slot = 0; slot < kBossMusicConfigs.size(); ++slot)
    {
        const BossMusicConfig& config = kBossMusicConfigs[slot];
        BossMusicState& state = mBossMusic[slot];


        int skillIndex = state.skillIndex;
        bool bossAlive = false;

        if (hasEnemies)
        {
            for (Entity entity : mWorld->GetEntitiesWithComponent<EnemyComponent>())
            {
                const EnemyComponent* enemy = mWorld->GetComponent<EnemyComponent>(entity);
                if (enemy == nullptr || enemy->mEnemyType != config.enemyType)
                    continue;
                if (enemy->mAnimState == static_cast<int>(EnemyAnimState::Dead))
                    continue;

                bossAlive = true;

                // 스킬 상태일 때만 갱신
                if (const int currentSkill = BossSkillIndexOf(enemy->mAnimState); currentSkill > 0)
                    skillIndex = currentSkill;
            }
        }

        if (!bossAlive)
        {
            if (state.playing)
            {
                AUDIOMANAGER.StopBGM(config.stem);
                state = BossMusicState{};
            }
            continue;
        }

        if (!state.playing)
        {

            if (!mBgmStartAligned)
                continue;


            AUDIOMANAGER.RequestBGM( config.eventPath, config.stem, { { config.paramName, 0.0f } }, true);

            state.playing = true;
            state.aligned = false;
            state.alignFrame = -1;
            state.resyncCooldown = 0.f;
            state.skillIndex = 0;
        }

        if (!state.aligned)
            AlignBossMusicToReferenceStem(config, state);

        if (state.skillIndex != skillIndex)
        {
            state.skillIndex = skillIndex;
            AUDIOMANAGER.SetBGMParam(
                config.paramName, config.stem, static_cast<float>(skillIndex), true);
            EngineLog::Write(
                EngineLog::Domain::AudioRuntime,
                "[BGM] ",
                config.paramName,
                "=",
                skillIndex);
        }
    }
}

void AudioSystem::StopAllBossMusic()
{
    for (size_t slot = 0; slot < kBossMusicConfigs.size(); ++slot)
    {
        AUDIOMANAGER.StopBGM(kBossMusicConfigs[slot].stem);
        mBossMusic[slot] = BossMusicState{};
    }
}


void AudioSystem::AlignBossMusicToReferenceStem(const BossMusicConfig& config, BossMusicState& state)
{
    // 타임라인 위상에 맞추기
    int referenceMs = 0;
    if (!AUDIOMANAGER.GetBGMTimelinePositionMs(SOUNDNAME::Elec, referenceMs))
        return;

    if (state.alignFrame < 0)
    {
        const float lead = kBgmAlignSeekFrames * DELTA_TIME;
        AUDIOMANAGER.SeekBGM(config.stem, WrapToLoop(referenceMs / 1000.f + lead));
        state.alignFrame = 0;
        return;
    }

    if (++state.alignFrame < kBgmAlignSeekFrames)
        return;

    AUDIOMANAGER.SetBGMPaused(config.stem, false);
    state.aligned = true;
    EngineLog::Write(
        EngineLog::Domain::AudioRuntime,
        "[T0] boss music aligned and resumed ",
        config.eventPath);
}

void AudioSystem::CorrectBossMusicDrift(float referencePhase, float pitch)
{
    for (size_t slot = 0; slot < kBossMusicConfigs.size(); ++slot)
    {
        const BossMusicConfig& config = kBossMusicConfigs[slot];
        BossMusicState& state = mBossMusic[slot];

        if (!state.playing || !state.aligned)
            continue;


        AUDIOMANAGER.SetBGMPitch(config.stem, pitch);

        if (state.resyncCooldown > 0.f)
        {
            state.resyncCooldown -= DELTA_TIME;
            continue;
        }

        int bossMs = 0;
        if (!AUDIOMANAGER.GetBGMTimelinePositionMs(config.stem, bossMs))
            continue;


        float drift = WrapToLoop(bossMs / 1000.f) - referencePhase;
        if (drift >  mLoopLen * 0.5f) drift -= mLoopLen;
        if (drift < -mLoopLen * 0.5f) drift += mLoopLen;


        if (fabsf(drift) <= kBossResyncSeconds)
            continue;

        AUDIOMANAGER.SeekBGM(config.stem, referencePhase);
        state.resyncCooldown = kBossResyncCooldown;
        EngineLog::Write(
            EngineLog::Domain::AudioRuntime,
            "[BGM] boss resync ",
            config.eventPath,
            " drift=",
            drift,
            "s");
    }
}

float AudioSystem::WrapToLoop(float seconds) const
{
    float phase = fmodf(seconds, mLoopLen);
    if (phase < 0.f)
        phase += mLoopLen;
    return phase;
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
