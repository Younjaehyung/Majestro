#pragma once
//#include <string>
//#include <vector>
//#include <unordered_map>
//#include <initializer_list>
#include <mutex>
#include <FMod/fmod_studio.h>
#include <FMod/fmod_studio.hpp>

#ifndef F_CALLBACK
#define F_CALLBACK
#endif

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

enum class SOUNDNAME {
	Ambient,
	Drum,
    Bass,
	Elec,
    End

};

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
    void PlayBGM(const char* eventPath, SOUNDNAME soundEnum);
    void StopBGM(SOUNDNAME soundEnum);

    void SetGlobalParam(const char* name, float v);
    void SetBusVolume(const char* busPath, float v);

    // 리스너는 매 프레임 동기화 (카메라 기반)
    void SetListener(const FMOD_3D_ATTRIBUTES& attr, int index = 0);

    // ... 기존
    void SetBGMParam(const char* name, SOUNDNAME soundEnum, float value, bool ignoreSeekSpeed = false);
    void SetBGMParamLabel(const char* name, SOUNDNAME soundEnum, const char* label, bool ignoreSeekSpeed = false);

    // 선택: 레벨 전환 시 묶음 프리로드/언로드
    void PreloadBanks(std::initializer_list<std::string> banks);
    void UnloadBanks(std::initializer_list<std::string> banks);
    bool GetBGMParam(const char* name, SOUNDNAME soundEnum, float& outValue, float* outFinalValue = nullptr) const;
    bool IsBGMPlaying(SOUNDNAME soundEnum) const;
    bool GetBGMEventPath(SOUNDNAME soundEnum, std::string& outEventPath) const;
    bool GetBGMTimelineMarker(SOUNDNAME soundEnum, std::string& outMarker) const;

    // ── 오디오 비주얼라이저용 FFT DSP ──────────────────────────────────
    // Initialize() 이후에 호출할 것.
    // windowSize: FFT 창 크기 (2의 거듭제곱, 클수록 주파수 해상도 ↑ / 시간 해상도 ↓)
    void InitSpectrumDSP(int windowSize = 1024);
    void ShutdownSpectrumDSP();

    // 매 프레임 스펙트럼 데이터 폴링. outSpectrum.size() == windowSize / 2
    // DSP가 초기화되지 않았거나 데이터가 없으면 false 반환
    bool GetSpectrumData(std::vector<float>& outSpectrum);

    // InitSpectrumDSP() 시 캐시된 FMOD 소프트웨어 샘플레이트 반환 (Hz)
    float GetSpectrumSampleRate() const { return mSpectrumSampleRate; }
    // ────────────────────────────────────────────────────────────────────

private:
    struct BGMCallbackData {
        AudioManager* owner = nullptr;
        SOUNDNAME soundEnum = SOUNDNAME::End;
    };
    static FMOD_RESULT F_CALLBACK OnBGMEventCallback(FMOD_STUDIO_EVENT_CALLBACK_TYPE type, FMOD_STUDIO_EVENTINSTANCE* event, void* parameters);
    void UpdateBGMTimelineMarker(SOUNDNAME soundEnum, const char* markerName);
    void ReleaseBGMInstance(FMOD::Studio::EventInstance*& instance, SOUNDNAME soundEnum);

    FmodBackend mFMOD;
    FMOD::Studio::EventInstance* mBGM = nullptr;
	std::vector<FMOD::Studio::EventInstance*> mAllBGM;
    std::vector<std::string> mCurrentBGMMarkers;
    mutable std::mutex mMarkerMutex;

    // FFT DSP
    FMOD::DSP* mSpectrumDSP      = nullptr;
    float      mSpectrumSampleRate = 44100.f;
};


