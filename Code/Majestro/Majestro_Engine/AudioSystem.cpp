#include "pch.h"
#include "Engine.h"
#include "AudioManager.h"
#include "AudioSystem.h"
#include "InputManager.h"

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

    if(INPUT.GetKeyDown(eKeyCode::_1)){
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
	}


}


void OnExplosion(float x, float y, float z) {
    FMOD_3D_ATTRIBUTES a{};
    a.position = { x, y, z };
    a.forward = { 0, 0, 1 };
    a.up = { 0, 1, 0 };
    AUDIOMANAGER.PlayOneShot3D("event:/Drum", a);
}
