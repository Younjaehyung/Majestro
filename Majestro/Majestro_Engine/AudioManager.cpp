#include "pch.h"
#include "AudioManager.h"

void AudioManager::Initialize() {

	FMOD::System_Create(&mSoundSystem, FMOD_VERSION);
	mSoundSystem->init(32, FMOD_INIT_NORMAL, NULL);
}

void AudioManager::Update()
{
	mSoundSystem->update();
}