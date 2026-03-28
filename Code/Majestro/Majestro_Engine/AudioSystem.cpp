#include "pch.h"
#include "Engine.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "InputManager.h"
#include "BeatSystem.h"
#include "PlayerComponent.h"

AudioSystem::AudioSystem(World* world) : System::System(world)
{
}

void AudioSystem::Initialize()
{
   
    AUDIOMANAGER.PreloadBanks({"MajestroBank.bank"}); // 필요 컨텐츠만
   
    AUDIOMANAGER.PlayBGM("event:/Elec", SOUNDNAME::Elec);              // BGM 시작
    AUDIOMANAGER.PlayBGM("event:/Bass" ,SOUNDNAME::Bass);              // BGM 시작
    AUDIOMANAGER.PlayBGM("event:/Drum", SOUNDNAME::Drum);              // BGM 시작
    

}

void AudioSystem::Update(float deltaTime)
{
    FMOD_3D_ATTRIBUTES listener{};
    listener.position = { 0, 0, 0 };
    listener.forward = { 0, 0, 1 };
    listener.up = { 0, 1, 0 };
    AUDIOMANAGER.SetListener(listener);
    AUDIOMANAGER.Update(deltaTime);

    time += deltaTime;

    /*if(INPUT.GetKeyDown(eKeyCode::_1)){
        AUDIOMANAGER.SetBGMParam("To Bass02", SOUNDNAME::Bass, 1.f, true);
	}
    if(INPUT.GetKeyDown(eKeyCode::_2)){
        AUDIOMANAGER.SetBGMParam("To Bass03", SOUNDNAME::Bass, 1.f, true);
	}

    if(INPUT.GetKeyDown(eKeyCode::_3)){
        AUDIOMANAGER.SetBGMParam("To Elec02", SOUNDNAME::Elec, 1.f, true);
	}
    if(INPUT.GetKeyDown(eKeyCode::_4)){
        AUDIOMANAGER.SetBGMParam("To Elec03", SOUNDNAME::Elec, 1.f, true);
	}

    if(INPUT.GetKeyDown(eKeyCode::_5)){
        AUDIOMANAGER.SetBGMParam("To Drum02", SOUNDNAME::Drum, 1.f, true);
	}
    if(INPUT.GetKeyDown(eKeyCode::_6)){
        AUDIOMANAGER.SetBGMParam("To Drum03", SOUNDNAME::Drum, 1.f, true);
	}*/

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
            playerComponent->mRhythm = playerComponent->mNextRhythm;
            playerComponent->mHasQueuedRhythmChange = false;
        }

        ApplyRhythmLayerByPlayerType(playerComponent->mPlayerType, playerComponent->mRhythm);
    }

}


void AudioSystem::ApplyRhythmLayerByPlayerType(uint8 playerType, uint8 rhythm)
{
    if (playerType > 2)
        return;

    switch (playerType)
    {
    case 0: // Drum player
        if (rhythm == 1)
            AUDIOMANAGER.SetBGMParam("To Drum03", SOUNDNAME::Drum, 1.f, true);
        else
            AUDIOMANAGER.SetBGMParam("To Drum02", SOUNDNAME::Drum, 1.f, true);
        break;

    case 1: // Bass player
        if (rhythm == 1)
            AUDIOMANAGER.SetBGMParam("To Bass03", SOUNDNAME::Bass, 1.f, true);
        else
            AUDIOMANAGER.SetBGMParam("To Bass02", SOUNDNAME::Bass, 1.f, true);
        break;

    case 2: // Elec player
        if (rhythm == 1)
            AUDIOMANAGER.SetBGMParam("To Elec03", SOUNDNAME::Elec, 1.f, true);
        else
            AUDIOMANAGER.SetBGMParam("To Elec02", SOUNDNAME::Elec, 1.f, true);
        break;

    default:
        return;
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