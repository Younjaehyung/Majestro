#include "pch.h"
#include "AudioManager.h"


namespace
{
    bool IsEventInstance3D(FMOD::Studio::EventInstance* instance)
    {
        if (instance == nullptr)
            return false;

        FMOD::Studio::EventDescription* desc = nullptr;
        if (instance->getDescription(&desc) != FMOD_OK || desc == nullptr)
            return false;

        bool is3D = false;
        if (desc->is3D(&is3D) != FMOD_OK)
            return false;

        return is3D;
    }
}


// ---------------------- FmodBackend ---------------------- //

std::string FmodBackend::JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    std::filesystem::path p = std::filesystem::path(a) / b;
    return p.string();
}

void FmodBackend::Initialize(const std::string& bankRoot, bool rightHanded3D) {
    mBankRoot = bankRoot;

    FMOD_CHECK(FMOD::Studio::System::create(&mStudio));

    FMOD_STUDIO_INITFLAGS studioFlags = FMOD_STUDIO_INIT_NORMAL;
    FMOD_INITFLAGS coreFlags = FMOD_INIT_NORMAL;
    if (rightHanded3D) coreFlags = FMOD_INIT_3D_RIGHTHANDED | coreFlags;

    // ���̽�/���� ���� ������Ʈ �����Ͽ� ���� ���� ����
    FMOD_CHECK(mStudio->initialize(1024, studioFlags, coreFlags, nullptr));
    FMOD_CHECK(mStudio->getCoreSystem(&mCore));

    // 3D ȯ�� �⺻ ������(���� ����)
    FMOD_CHECK(mCore->set3DSettings(1.0f, 1.0f, 1.0f));
}

void FmodBackend::Shutdown() {
    // ��ũ ��ε�
    for (auto& kv : mBanks) {
        if (kv.second) {
            kv.second->unloadSampleData();
            kv.second->unload();
        }
    }
    mBanks.clear();

    if (mStudio) {
        mStudio->unloadAll();
        mStudio->release();
        mStudio = nullptr;
        mCore = nullptr;
    }
    mEventCache.clear();
}

void FmodBackend::Update() {
    FMOD_CHECK(mStudio->update());
}

void FmodBackend::LoadBank(const std::string& bank, bool preloadSampleData) {
    if (mBanks.count(bank)) return;

    FMOD::Studio::Bank* b = nullptr;
    auto full = JoinPath(mBankRoot, bank);
    FMOD_CHECK(mStudio->loadBankFile(full.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &b));
    mBanks.emplace(bank, b);

    if (preloadSampleData) {
        FMOD_CHECK(b->loadSampleData());
    }
}

void FmodBackend::UnloadBank(const std::string& bank) {
    auto it = mBanks.find(bank);
    if (it == mBanks.end()) return;
    FMOD::Studio::Bank* b = it->second;
    if (b) {
        b->unloadSampleData();
        b->unload();
    }
    mBanks.erase(it);
}

FMOD::Studio::EventDescription* FmodBackend::GetOrCacheEventDesc(const char* eventPath) {
    auto it = mEventCache.find(eventPath);
    if (it != mEventCache.end()) return it->second;

    FMOD::Studio::EventDescription* desc = nullptr;
    FMOD_RESULT r = mStudio->getEvent(eventPath, &desc);
    if (r != FMOD_OK) {
        // strings.bank �̷ε� or �ش� �̺�Ʈ ������ ��ũ �̷ε��� �� ����
        FMOD_CHECK(r);
    }
    mEventCache.emplace(eventPath, desc);
    return desc;
}

FMOD::Studio::EventInstance* FmodBackend::CreateInstance(const char* eventPath) {
    FMOD::Studio::EventDescription* desc = GetOrCacheEventDesc(eventPath);
    FMOD::Studio::EventInstance* inst = nullptr;
    FMOD_CHECK(desc->createInstance(&inst));
    return inst;
}

// ---------------------- AudioManager ---------------------- //

void AudioManager::Initialize(const std::string& bankRoot) {
    // ������ RH ��ǥ��� true
    mFMOD.Initialize(bankRoot, /*rightHanded3D=*/false);

    // ���ڿ� ��ȸ�� �ʼ�
    mFMOD.LoadBank("Master.bank");
    mFMOD.LoadBank("Master.strings.bank", /*preloadSampleData=*/false);

	mAllBGM.resize(static_cast<size_t>(SOUNDNAME::End) + 1, nullptr);
    mCurrentBGMMarkers.resize(static_cast<size_t>(SOUNDNAME::End) + 1);

	// 모든 mAllBGM을 nullptr로 초기화


}

void AudioManager::Shutdown() {

    // FFT DSP가 남아 있으면 먼저 해제 (Studio 릴리즈 전에 수행해야 함)
    ShutdownSpectrumDSP();

    StopAllLoops();

    if (!mAllBGM.empty()) {
        for (size_t i = 0; i < mAllBGM.size(); ++i) {
            if (!mAllBGM[i]) {
                continue;
            }
            ReleaseBGMInstance(mAllBGM[i], static_cast<SOUNDNAME>(i));
        }
        mAllBGM.clear();
    }
    mCurrentBGMMarkers.clear();

    mFMOD.Shutdown();
}

void AudioManager::Update(float dt) {
    mFMOD.Update();

    // 아무도 스펙트럼을 읽지 않는 씬(게임 플레이 등)에서는 FFT DSP를 bypass시켜  믹서 스레드의 FFT 연산을 중단
    constexpr float kSpectrumIdleBypassSec = 1.f;
    if (mSpectrumDSP && !mSpectrumBypassed) {
        mSpectrumIdleTime += dt;
        if (mSpectrumIdleTime > kSpectrumIdleBypassSec) {
            mSpectrumDSP->setBypass(true);
            mSpectrumBypassed = true;
        }
    }
}

void AudioManager::PreloadBanks(std::initializer_list<std::string> banks) {
    for (auto& b : banks) mFMOD.LoadBank(b);
}
void AudioManager::UnloadBanks(std::initializer_list<std::string> banks) {
    for (auto& b : banks) mFMOD.UnloadBank(b);
}

void AudioManager::PlayOneShot(const char* eventPath) {
    auto* inst = mFMOD.CreateInstance(eventPath);
    FMOD_CHECK(inst->start());
    FMOD_CHECK(inst->release()); // fire-and-forget
}

void AudioManager::PlayOneShot3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr) {
    auto* inst = mFMOD.CreateInstance(eventPath);
    FMOD_CHECK(inst->set3DAttributes(&attr));
    FMOD_CHECK(inst->start());
    FMOD_CHECK(inst->release());
}

// ── 상태 연동 루프 사운드 ──────────────────────────────────────────────

SfxHandle AudioManager::StartLoop(const char* eventPath) {
    auto* inst = mFMOD.CreateInstance(eventPath);
    FMOD_CHECK(inst->start());

    const SfxHandle handle = mNextLoopHandle++;
    if (mNextLoopHandle == 0) mNextLoopHandle = 1; // 랩어라운드 시 invalid(0) 회피
    mLoopInstances[handle] = inst;
    return handle;
}

SfxHandle AudioManager::StartLoop3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr) {
    auto* inst = mFMOD.CreateInstance(eventPath);
    FMOD_CHECK(inst->set3DAttributes(&attr));
    FMOD_CHECK(inst->start());

    const SfxHandle handle = mNextLoopHandle++;
    if (mNextLoopHandle == 0) mNextLoopHandle = 1;
    mLoopInstances[handle] = inst;
    return handle;
}

void AudioManager::StopLoop(SfxHandle handle, bool allowFadeout) {
    auto it = mLoopInstances.find(handle);
    if (it == mLoopInstances.end()) return;

    // 페이드아웃 길이는 FMOD Studio 이벤트의 AHDSR Release가 결정
    it->second->stop(allowFadeout ? FMOD_STUDIO_STOP_ALLOWFADEOUT : FMOD_STUDIO_STOP_IMMEDIATE);
    it->second->release();
    mLoopInstances.erase(it);
}

void AudioManager::SetLoop3DAttributes(SfxHandle handle, const FMOD_3D_ATTRIBUTES& attr) {
    auto it = mLoopInstances.find(handle);
    if (it == mLoopInstances.end()) return;
    it->second->set3DAttributes(&attr);
}

void AudioManager::SetLoopParam(SfxHandle handle, const char* name, float value) {
    auto it = mLoopInstances.find(handle);
    if (it == mLoopInstances.end()) return;
    it->second->setParameterByName(name, value);
}

void AudioManager::StopAllLoops() {
    for (auto& kv : mLoopInstances) {
        if (kv.second) {
            kv.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            kv.second->release();
        }
    }
    mLoopInstances.clear();
}

void AudioManager::PlayBGM(const char* eventPath, SOUNDNAME soundEnum) {
    // ���� BGM ����
    //if (mAllBGM[soundEnum]) {
    //    mAllBGM[soundEnum]->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    //    mAllBGM[soundEnum]->release();
    //    mAllBGM[soundEnum] = nullptr;
    //} // to-do
	uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size()) return;

    // 씬 전환 등으로 동일 BGM 슬롯이 재생될 경우 기존 인스턴스를 정리해 중첩 재생을 막는다.
    if (mAllBGM[idx]) {
        ReleaseBGMInstance(mAllBGM[idx], soundEnum);
    }

    mBGM = mFMOD.CreateInstance(eventPath);
	mAllBGM[idx] = mBGM;
    auto* callbackData = new BGMCallbackData{ this, soundEnum };
    FMOD_CHECK(mBGM->setUserData(callbackData));
    FMOD_CHECK(mBGM->setCallback(&AudioManager::OnBGMEventCallback, FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_MARKER));
    {
        std::lock_guard<std::mutex> lock(mMarkerMutex);
        mCurrentBGMMarkers[idx].clear();
    }
    // �ʿ� �� �Ķ����/���� ����� ����
    // BGM 이벤트가 3D여도 리스너 위치에서 재생해 거리 감쇠 없이 2D처럼 들리게 한다.
    if (mHasListenerAttr) {
        SyncBGMToListener(mListenerAttr);
    }
    FMOD_CHECK(mBGM->start());
    // �����ϸ� ������ ���̹Ƿ� release�� ���⼭ ���� ����(Shutdown/StopBGM����)
}

void AudioManager::StopBGM(SOUNDNAME soundEnum) {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size()) return;

    if (!mAllBGM[idx]) return;
    ReleaseBGMInstance(mAllBGM[idx], soundEnum);
}

void AudioManager::RequestBGM(const char* eventPath, SOUNDNAME soundEnum) {
    // 씬 전환 시 같은 곡이면 재시작하지 않고 그대로 이어간다.

    std::string currentPath;
    if (IsBGMPlaying(soundEnum) &&
        GetBGMEventPath(soundEnum, currentPath) &&
        currentPath == eventPath)
    {
        return;
    }

    PlayBGM(eventPath, soundEnum);
}

void AudioManager::SetGlobalParam(const char* name, float v) {
    FMOD_CHECK(mFMOD.GetStudio()->setParameterByName(name, v));
}

void AudioManager::SetBusVolume(const char* busPath, float v) {
    FMOD::Studio::Bus* bus = nullptr;
    FMOD_CHECK(mFMOD.GetStudio()->getBus(busPath, &bus));
    FMOD_CHECK(bus->setVolume(v));
}

void AudioManager::SetListener(const FMOD_3D_ATTRIBUTES& attr, int index) {

    FMOD_CHECK(mFMOD.GetStudio()->setListenerAttributes(index, &attr));

    if (index == 0) {
        mListenerAttr = attr;
        mHasListenerAttr = true;
        SyncBGMToListener(attr);
    }
}

void AudioManager::SetBGMParam(const char* name, SOUNDNAME soundEnum, float value, bool ignoreSeekSpeed) {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (!mAllBGM[idx]) return; // ���� BGM�� ���۵��� �ʾҴٸ� ����
    FMOD_CHECK(mAllBGM[idx]->setParameterByName(name, value, ignoreSeekSpeed));
}

void AudioManager::SetBGMParamLabel(const char* name, SOUNDNAME soundEnum, const char* label, bool ignoreSeekSpeed) {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (!mAllBGM[idx]) return;
    // ����(Discrete Labeled) �Ķ���͸� ���ڿ� �󺧷� ���� ����
    FMOD_CHECK(mAllBGM[idx]->setParameterByNameWithLabel(name, label, ignoreSeekSpeed));
}

bool AudioManager::GetBGMParam(const char* name, SOUNDNAME soundEnum, float& outValue, float* outFinalValue) const {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size() || !mAllBGM[idx]) {
        return false;
    }

    float finalValue = 0.f;
    FMOD_RESULT result = mAllBGM[idx]->getParameterByName(name, &outValue, &finalValue);
    if (result != FMOD_OK) {
        return false;
    }

    if (outFinalValue) {
        *outFinalValue = finalValue;
    }
    return true;
}

bool AudioManager::IsBGMPlaying(SOUNDNAME soundEnum) const {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size() || !mAllBGM[idx]) {
        return false;
    }

    FMOD_STUDIO_PLAYBACK_STATE state = FMOD_STUDIO_PLAYBACK_STOPPED;
    FMOD_CHECK(mAllBGM[idx]->getPlaybackState(&state));
    return state == FMOD_STUDIO_PLAYBACK_PLAYING || state == FMOD_STUDIO_PLAYBACK_STARTING;
}

bool AudioManager::GetBGMEventPath(SOUNDNAME soundEnum, std::string& outEventPath) const {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size() || !mAllBGM[idx]) {
        return false;
    }

    FMOD::Studio::EventDescription* desc = nullptr;
    FMOD_RESULT result = mAllBGM[idx]->getDescription(&desc);
    if (result != FMOD_OK || desc == nullptr) {
        return false;
    }

    int requiredSize = 0;
    result = desc->getPath(nullptr, 0, &requiredSize);
    if (result != FMOD_OK && result != FMOD_ERR_TRUNCATED) {
        return false;
    }
    if (requiredSize <= 0) {
        return false;
    }

    std::string eventPath(static_cast<size_t>(requiredSize), '\0');
    result = desc->getPath(eventPath.data(), requiredSize, &requiredSize);
    if (result != FMOD_OK) {
        return false;
    }

    if (requiredSize > 0 && eventPath[static_cast<size_t>(requiredSize - 1)] == '\0') {
        eventPath.resize(static_cast<size_t>(requiredSize - 1));
    }
    else {
        eventPath.resize(static_cast<size_t>(requiredSize));
    }

    outEventPath = std::move(eventPath);
    return true;
}

bool AudioManager::GetBGMTimelineMarker(SOUNDNAME soundEnum, std::string& outMarker) const {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mCurrentBGMMarkers.size()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mMarkerMutex);
    if (mCurrentBGMMarkers[idx].empty()) {
        return false;
    }

    outMarker = mCurrentBGMMarkers[idx];
    return true;
}

FMOD_RESULT F_CALLBACK AudioManager::OnBGMEventCallback(
    FMOD_STUDIO_EVENT_CALLBACK_TYPE type,
    FMOD_STUDIO_EVENTINSTANCE* event,
    void* parameters) {
    if (type != FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_MARKER || event == nullptr || parameters == nullptr) {
        return FMOD_OK;
    }

    auto* instance = reinterpret_cast<FMOD::Studio::EventInstance*>(event);
    void* userData = nullptr;
    if (instance->getUserData(&userData) != FMOD_OK || userData == nullptr) {
        return FMOD_OK;
    }

    auto* callbackData = static_cast<BGMCallbackData*>(userData);
    if (callbackData->owner == nullptr) {
        return FMOD_OK;
    }

    auto* marker = static_cast<FMOD_STUDIO_TIMELINE_MARKER_PROPERTIES*>(parameters);
    callbackData->owner->UpdateBGMTimelineMarker(callbackData->soundEnum, marker->name);
    return FMOD_OK;
}

void AudioManager::UpdateBGMTimelineMarker(SOUNDNAME soundEnum, const char* markerName) {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mCurrentBGMMarkers.size()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mMarkerMutex);
    mCurrentBGMMarkers[idx] = (markerName != nullptr) ? markerName : "";
}

void AudioManager::SyncBGMToListener(const FMOD_3D_ATTRIBUTES& listenerAttr) {
	// 2D처럼 들려야 하는 BGM 이벤트도 3D로 만들어졌을 수 있으므로, 리스너 위치에 맞춰 3D 속성을 업데이트한다.

    for (FMOD::Studio::EventInstance* bgm : mAllBGM) {
        if (bgm == nullptr || IsEventInstance3D(bgm) == false) {
            continue;
        }

        
        FMOD_3D_ATTRIBUTES bgmAttr = listenerAttr;
        bgmAttr.velocity = { 0, 0, 0 };
        FMOD_CHECK(bgm->set3DAttributes(&bgmAttr));
    }
}

void AudioManager::ReleaseBGMInstance(FMOD::Studio::EventInstance*& instance, SOUNDNAME soundEnum) {
    if (!instance) {
        return;
    }

    void* userData = nullptr;
    instance->setCallback(nullptr, FMOD_STUDIO_EVENT_CALLBACK_TIMELINE_MARKER);
    if (instance->getUserData(&userData) == FMOD_OK && userData != nullptr) {
        delete static_cast<BGMCallbackData*>(userData);
        instance->setUserData(nullptr);
    }
    instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    instance->release();
    instance = nullptr;

    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx < mCurrentBGMMarkers.size()) {
        std::lock_guard<std::mutex> lock(mMarkerMutex);
        mCurrentBGMMarkers[idx].clear();
    }
}


// ── 오디오 비주얼라이저용 FFT DSP ───────────────────────────────────────

void AudioManager::InitSpectrumDSP(int windowSize)
{
    if (mSpectrumDSP)
        return;  // 이미 초기화됨

    // 소프트웨어 샘플레이트 캐시
    int sr = 84100;
    FMOD_CHECK(mFMOD.GetCore()->getSoftwareFormat(&sr, nullptr, nullptr));
    mSpectrumSampleRate = static_cast<float>(sr);

    // 마스터 채널 그룹에 FFT DSP 삽입 (Studio 이벤트도 Core 채널 그룹을 통과)
    FMOD::ChannelGroup* masterGroup = nullptr;
    FMOD_CHECK(mFMOD.GetCore()->getMasterChannelGroup(&masterGroup));

    FMOD::DSP* dsp = nullptr;
    FMOD_CHECK(mFMOD.GetCore()->createDSPByType(FMOD_DSP_TYPE_FFT, &dsp));
    FMOD_CHECK(dsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, windowSize));

    // DSP chain 맨 앞에 추가 (전체 출력 오디오 캡처)
    FMOD_CHECK(masterGroup->addDSP(0, dsp));
    mSpectrumDSP = dsp;
}

void AudioManager::ShutdownSpectrumDSP()
{
    if (!mSpectrumDSP)
        return;

    FMOD::ChannelGroup* masterGroup = nullptr;
    if (mFMOD.GetCore())
        mFMOD.GetCore()->getMasterChannelGroup(&masterGroup);

    if (masterGroup)
        masterGroup->removeDSP(mSpectrumDSP);

    mSpectrumDSP->release();
    mSpectrumDSP = nullptr;
    mSpectrumIdleTime = 0.f;
    mSpectrumBypassed = false;
}

bool AudioManager::GetSpectrumData(std::vector<float>& outSpectrum)
{
    if (!mSpectrumDSP)
        return false;

    
    mSpectrumIdleTime = 0.f;
    if (mSpectrumBypassed)
    {
        mSpectrumDSP->setBypass(false);
        mSpectrumBypassed = false;
        return false;
    }

    FMOD_DSP_PARAMETER_FFT* fftData = nullptr;
    FMOD_RESULT r = mSpectrumDSP->getParameterData(
        FMOD_DSP_FFT_SPECTRUMDATA,
        reinterpret_cast<void**>(&fftData),
        nullptr, nullptr, 0);

    if (r != FMOD_OK || !fftData || fftData->numchannels == 0 || fftData->length == 0)
        return false;

    int len = fftData->length;
    outSpectrum.resize(static_cast<size_t>(len));

    // 채널 평균 → 모노 스펙트럼
    for (int i = 0; i < len; i++)
    {
        float sum = 0.f;
        for (int ch = 0; ch < fftData->numchannels; ch++)
            sum += fftData->spectrum[ch][i];
        outSpectrum[i] = sum / static_cast<float>(fftData->numchannels);
    }
    return true;
}

// ────────────────────────────────────────────────────────────────────────

