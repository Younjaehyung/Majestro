#pragma once
#include <FMod/fmod_studio.hpp>
#include <FMod/fmod_errors.h>

inline void FMOD_CHECK(FMOD_RESULT r) {
    if (r != FMOD_OK) {
        // 여기서 엔진 로거로 치환하세요.
        // 예: LOGF("FMOD error: %s (%d)", FMOD_ErrorString(r), (int)r);
        throw std::runtime_error(FMOD_ErrorString(r));
    }
}

class AudioSystem
{
public:
    void Initialize(const std::string& bankRoot, bool rightHanded3D = true);
    void Shutdown();
    void Update(float /*dt*/);

    // Listener
    void SetListenerAttributes(const FMOD_3D_ATTRIBUTES& attr, int index = 0);

    // Banks
    void LoadBank(const std::string& bankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags = FMOD_STUDIO_LOAD_BANK_NORMAL, bool preloadSampleData = true);
    void UnloadBank(const std::string& bankName);

    // Events
    FMOD::Studio::EventInstance* CreateInstance(const char* eventPath);
    void PlayOneShot(const char* eventPath);
    void PlayOneShot3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr);
    void ReleaseInstance(FMOD::Studio::EventInstance* inst); // 보통 start 후 release 패턴

    // Buses / VCAs
    FMOD::Studio::Bus* GetBus(const char* busPath);
    FMOD::Studio::VCA* GetVCA(const char* vcaPath);

    // Access
    FMOD::Studio::System* Studio() const { return mStudio; }
    FMOD::System* Core()   const { return mCore; }

private:
    std::string mBankRoot;
    FMOD::Studio::System* mStudio = nullptr;
    FMOD::System* mCore = nullptr;

    std::unordered_map<std::string, FMOD::Studio::Bank*> mBanks;

};

