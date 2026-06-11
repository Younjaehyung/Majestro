#include "pch.h"
#include "Engine.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "BeatSystem.h"
#include "PlayerComponent.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "TagComponent.h"

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


    AUDIOMANAGER.RequestBGM("event:/Elec", SOUNDNAME::Elec);
    AUDIOMANAGER.RequestBGM("event:/Bass", SOUNDNAME::Bass);
    AUDIOMANAGER.RequestBGM("event:/Drum", SOUNDNAME::Drum);
    

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

    time += deltaTime;

    if (!mWorld->HasComponentPool<MainPlayerComponent>())
        return;

    auto playerEntities = mWorld->GetEntitiesWithComponent<MainPlayerComponent>();
    if (playerEntities.empty())
        return;

    for (Entity playerEntity : playerEntities)
    {
        MainPlayerComponent* playerComponent = mWorld->GetComponent<MainPlayerComponent>(playerEntity);
        if (playerComponent == nullptr)
            continue;

        if (playerComponent->mHasQueuedRhythmChange)
        {
            ApplyRhythmLayerByPlayerType(playerComponent->mPlayerType, playerComponent->mNextRhythm);
            playerComponent->mHasQueuedRhythmChange = false;
        }

        if (playerComponent->mRhythm != playerComponent->mNextRhythm) {
            
            if (IsCurrentRhythmMatched(playerComponent->mPlayerType, playerComponent->mNextRhythm))
            {
                SendRhythmChangedPacket(mWorld, playerEntity, playerComponent->mRhythm, playerComponent->mNextRhythm, playerComponent->mPlayerType);
                playerComponent->mRhythm = playerComponent->mNextRhythm;

                //cout << "rythm change succese :" << (int)playerComponent->mRhythm << endl;
            }
        }

    }

}


void AudioSystem::ApplyRhythmLayerByPlayerType(uint8 playerType, uint8 rhythm)
{
    if (playerType > 2)
        return;


    switch (playerType)
    {
    case 0: // Drum player
        switch (rhythm)
        {
        case 0:
            AUDIOMANAGER.SetBGMParam("NextDrum", SOUNDNAME::Drum, 0.f, true);
            break;
        case 1:
            AUDIOMANAGER.SetBGMParam("NextDrum", SOUNDNAME::Drum, 0.25f, true);
            break;
        case 2:
            AUDIOMANAGER.SetBGMParam("NextDrum", SOUNDNAME::Drum, 0.50f, true);
            break;
        case 3:
            AUDIOMANAGER.SetBGMParam("NextDrum", SOUNDNAME::Drum, 0.75f, true);
            break;
        }
        break;

    case 1: // Bass player
        switch (rhythm)
        {
        case 0:
            AUDIOMANAGER.SetBGMParam("NextBass", SOUNDNAME::Bass, 0.f, true);
            break;
        case 1:
            AUDIOMANAGER.SetBGMParam("NextBass", SOUNDNAME::Bass, 0.25f, true);
            break;
        case 2:
            AUDIOMANAGER.SetBGMParam("NextBass", SOUNDNAME::Bass, 0.50f, true);
            break;
        case 3:
            AUDIOMANAGER.SetBGMParam("NextBass", SOUNDNAME::Bass, 0.75f, true);
            break;
        }
        break;

    case 2: // Elec player
        switch (rhythm)
        {
        case 0:
            AUDIOMANAGER.SetBGMParam("NextElec", SOUNDNAME::Elec, 0.f, true);
            break;
        case 1:
            AUDIOMANAGER.SetBGMParam("NextElec", SOUNDNAME::Elec, 0.25f, true);
            break;
        case 2:
            AUDIOMANAGER.SetBGMParam("NextElec", SOUNDNAME::Elec, 0.50f, true);
            break;
        case 3:
            AUDIOMANAGER.SetBGMParam("NextElec", SOUNDNAME::Elec, 0.75f, true);
            break;
        }
        break;

    default:
        return;
    }

}

bool AudioSystem::IsCurrentRhythmMatched(uint8 playerType, uint8 rhythm) const
{
    const SOUNDNAME soundEnum = GetSoundNameByPlayerType(playerType);
    if (soundEnum == SOUNDNAME::End) {
        return false;
    }

    std::string currentMarker;
    if (!AUDIOMANAGER.GetBGMTimelineMarker(soundEnum, currentMarker))
        return false;

    const char* expectedMarker = GetExpectedMarkerByPlayerType(playerType, rhythm);
    if (expectedMarker == nullptr) {
        return false;
    }

    return currentMarker == expectedMarker;
}

SOUNDNAME AudioSystem::GetSoundNameByPlayerType(uint8 playerType)
{
    switch (playerType)
    {
    case 0: return SOUNDNAME::Drum;
    case 1: return SOUNDNAME::Bass;
    case 2: return SOUNDNAME::Elec;
    default: return SOUNDNAME::End;
    }
}

const char* AudioSystem::GetExpectedMarkerByPlayerType(uint8 playerType, uint8 rhythm)
{
    switch (playerType) {
    case 0:
        switch (rhythm % 4) {
        case 0: return "Drum00";
        case 1: return "Drum01";
        case 2: return "Drum02";
        case 3: return "Drum03";
        default: return nullptr;
        }
    case 1:
        switch (rhythm % 4) {
        case 0: return "Bass00";
        case 1: return "Bass01";
        case 2: return "Bass02";
        case 3: return "Bass03";
        default: return nullptr;
        }
    case 2:
        switch (rhythm % 4) {
        case 0: return "Elec00";
        case 1: return "Elec01";
        case 2: return "Elec02";
        case 3: return "Elec03";
        default: return nullptr;
        }
    default:
        return nullptr;
    }
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