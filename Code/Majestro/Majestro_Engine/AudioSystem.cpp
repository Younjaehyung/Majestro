#include "pch.h"
#include "Engine.h"
#include "AudioManager.h"
#include "AudioSystem.h"

AudioSystem::AudioSystem(World* world) : System::System(world)
{
}

void AudioSystem::Initialize()
{
   
    AUDIOMANAGER.PreloadBanks({"DrumBank.bank"}); // 필요 컨텐츠만
    AUDIOMANAGER.PlayBGM("event:/Drum");              // BGM 시작
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
    if (time > 12.0f) {
        AUDIOMANAGER.SetBGMParam("To Drum2Yeah", 1.f,true);
    }
    if(time > 30.0f){
        AUDIOMANAGER.SetBGMParam("To Drum3Yeah", 1.f, true);
    } 

}


void OnExplosion(float x, float y, float z) {
    FMOD_3D_ATTRIBUTES a{};
    a.position = { x, y, z };
    a.forward = { 0, 0, 1 };
    a.up = { 0, 1, 0 };
    AUDIOMANAGER.PlayOneShot3D("event:/Drum", a);
}
