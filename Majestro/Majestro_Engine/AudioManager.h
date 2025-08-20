#pragma once

namespace FMOD {
	class System;
	class Sound;
	class Channel;
}

class FmodBackend {
public:
    void Initialize(const std::string& bankRoot, bool rightHanded3D);
    void Shutdown();
    void Update();

    // Studio/Core 핸들 접근
    FMOD::Studio::System* Studio() const;
    FMOD::System* Core()   const;

    // Bank/Event/Bus/VCA… (필요 최소만)
    void LoadBank(const std::string& bank, bool preloadSampleData = true);
    FMOD::Studio::EventInstance* CreateInstance(const char* eventPath);
    // ...
};


class AudioManager {
public:
    void Initialize(const std::string& bankRoot);
    void Shutdown();
    void Update(float dt);

    void PlayOneShot(const char* eventPath);
    void PlayOneShot3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr);
    void PlayBGM(const char* eventPath);
    void StopBGM();

    void SetGlobalParam(const char* name, float v);
    void SetBusVolume(const char* busPath, float v);

    // 리스너는 매 프레임 동기화 (카메라 기반)
    void SetListener(const FMOD_3D_ATTRIBUTES& attr, int index = 0);

    FmodBackend& Backend(); // 필요 시 하위로 빠질 수 있게

private:
    FmodBackend mFMOD;
    // 스레드세이프 큐/핸들 캐시 등
};


