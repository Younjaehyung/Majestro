#pragma once
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
    BrassBoss,   // Brass 보스 전용 음악 (BrassParam 으로 스킬 연출)
    End

};

// 음량 카테고리
enum class AudioCategory : uint8 {
    Master,   // bus:/        (전체)
    Vfx,      // bus:/SFX     (효과음)
    Ambient,  // bus:/Ambient (환경/배경음)
    Count
};

// 상태 연동 루프 사운드용 불투명 핸들. 0 = invalid.
using SfxHandle = uint32;

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

    // 효과음
    void PlayOneShot(const char* eventPath);
    void PlayOneShot3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr);

    // 배경음
    void PlayBGM(const char* eventPath, SOUNDNAME soundEnum);
    void StopBGM(SOUNDNAME soundEnum);

    // 같은 슬롯에 같은 곡이 이미 재생 중이면 아무것도 하지 않는다(씬 전환 시 끊김 없이 이어짐).  다른 곡이면 교체한다
    void RequestBGM(const char* eventPath, SOUNDNAME soundEnum);

    // 루프 효과음
    SfxHandle StartLoop(const char* eventPath);
    SfxHandle StartLoop3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr);

    void StopLoop(SfxHandle handle, bool allowFadeout = true);
    void SetLoop3DAttributes(SfxHandle handle, const FMOD_3D_ATTRIBUTES& attr);
    void SetLoopParam(SfxHandle handle, const char* name, float value);
    void StopAllLoops();

    void SetGlobalParam(const char* name, float v);
    void SetBusVolume(const char* busPath, float v);

    // 카테고리 음량 (Setting UI) 
    void  SetCategoryVolume(AudioCategory cat, float v01);
    float GetCategoryVolume(AudioCategory cat) const;
    void  ApplyCategoryVolumes();   // 보관 중인 음량을 FMOD 버스에 일괄 반영

    // 리스너는 매 프레임 동기화 (카메라 기반)
    void SetListener(const FMOD_3D_ATTRIBUTES& attr, int index = 0);

    // ... 기존
    void SetBGMParam(const char* name, SOUNDNAME soundEnum, float value, bool ignoreSeekSpeed = false);
    void SetBGMParamLabel(const char* name, SOUNDNAME soundEnum, const char* label, bool ignoreSeekSpeed = false);

    // FMOD 점프 검증
    void SeekBGM(SOUNDNAME soundEnum, float seconds);                       // setTimelinePosition(ms)
    bool GetBGMTimelinePositionMs(SOUNDNAME soundEnum, int& outMs) const;   // getTimelinePosition
    void SetBGMPitch(SOUNDNAME soundEnum, float pitch);                     // setPitch (드리프트 피치 너지)
    void SetBGMPaused(SOUNDNAME soundEnum, bool paused);                    // setPaused
    void SetBGMVolume(SOUNDNAME soundEnum, float volume);                   // 개별 음악 음량 배율 설정
    
    void DebugStartSeekProbe(float targetSeconds);                          // 여러 프레임 위치를 로그
        
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

    // 시킹 검증 프로브: 시킹 직후 N 프레임 동안 타임라인 위치를 추적.
    struct SeekProbe {
        bool  active = false;
        int   framesLeft = 0;
        float requestedSec = 0.f;
        int   lastMs[static_cast<size_t>(SOUNDNAME::End)] = {};
    };

    SeekProbe mSeekProbe;
    void PollSeekProbe();
    void ReleaseBGMInstance(FMOD::Studio::EventInstance*& instance, SOUNDNAME soundEnum);
    void SyncBGMToListener(const FMOD_3D_ATTRIBUTES& listenerAttr);

    FmodBackend mFMOD;

    // 카테고리별 음량 (0~1).
    float mCategoryVolume[static_cast<size_t>(AudioCategory::Count)] = { 1.f, 1.f, 1.f };   // 인덱스 = AudioCategory.
    static const char* BusPathOf(AudioCategory cat);

    // 상태 연동 루프 인스턴스 (핸들 <-> 인스턴스)
    std::unordered_map<SfxHandle, FMOD::Studio::EventInstance*> mLoopInstances;
    SfxHandle mNextLoopHandle = 1;

    FMOD::Studio::EventInstance* mBGM = nullptr;
	std::vector<FMOD::Studio::EventInstance*> mAllBGM;
    std::vector<std::string> mCurrentBGMMarkers;
    mutable std::mutex mMarkerMutex;
    FMOD_3D_ATTRIBUTES mListenerAttr{};
    bool mHasListenerAttr = false;

    // FFT DSP
    FMOD::DSP* mSpectrumDSP      = nullptr;
    float      mSpectrumSampleRate = 44100.f;

	// FFT 연산 자동 정지 방지용 타이머.
    float      mSpectrumIdleTime = 0.f;
    bool       mSpectrumBypassed = false;
};


