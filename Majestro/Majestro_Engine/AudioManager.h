#pragma once


inline void FMOD_CHECK(FMOD_RESULT r) {
    if (r != FMOD_OK) {
        throw std::runtime_error(FMOD_ErrorString(r));
    }
}


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
    FMOD::Studio::System* GetStudio() const { return mStudio; }
    FMOD::System* GetCore()   const { return mCore; }

    // Bank/Event/Bus/VCA… (필요 최소만)
    void LoadBank(const std::string& bank, bool preloadSampleData = true);
    void UnloadBank(const std::string& bank);
    FMOD::Studio::EventInstance* CreateInstance(const char* eventPath);


private:
    std::string mBankRoot;
    FMOD::Studio::System* mStudio = nullptr;
    FMOD::System* mCore = nullptr;

    std::unordered_map<std::string, FMOD::Studio::Bank*> mBanks;
    std::unordered_map<std::string, FMOD::Studio::EventDescription*> mEventCache;

    FMOD::Studio::EventDescription* GetOrCacheEventDesc(const char* eventPath);
    static std::string JoinPath(const std::string& a, const std::string& b);
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

    // FmodBackend& Backend(); // 필요 시 하위로 빠질 수 있게


    // ... 기존
    void SetBGMParam(const char* name, float value, bool ignoreSeekSpeed = false);
    void SetBGMParamLabel(const char* name, const char* label, bool ignoreSeekSpeed = false);



        // 선택: 레벨 전환 시 묶음 프리로드/언로드
    void PreloadBanks(std::initializer_list<std::string> banks);
    void UnloadBanks(std::initializer_list<std::string> banks);
private:
    FmodBackend mFMOD;
    FMOD::Studio::EventInstance* mBGM = nullptr;
    // 스레드세이프 큐/핸들 캐시 등

    
};


