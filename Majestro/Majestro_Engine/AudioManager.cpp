#include "pch.h"
#include "AudioManager.h"


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

    // 보이스/버퍼 등은 프로젝트 스케일에 맞춰 조정 가능
    FMOD_CHECK(mStudio->initialize(1024, studioFlags, coreFlags, nullptr));
    FMOD_CHECK(mStudio->getCoreSystem(&mCore));

    // 3D 환경 기본 스케일(미터 단위)
    FMOD_CHECK(mCore->set3DSettings(1.0f, 1.0f, 1.0f));
}

void FmodBackend::Shutdown() {
    // 뱅크 언로드
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
        // strings.bank 미로드 or 해당 이벤트 미포함 뱅크 미로드일 수 있음
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
    // 엔진이 RH 좌표계면 true
    mFMOD.Initialize(bankRoot, /*rightHanded3D=*/false);

    // 문자열 조회에 필수
    mFMOD.LoadBank("Master.bank");
    mFMOD.LoadBank("Master.strings.bank", /*preloadSampleData=*/false);
}

void AudioManager::Shutdown() {
    if (mBGM) {
        mBGM->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        mBGM->release();
        mBGM = nullptr;
    }
    mFMOD.Shutdown();
}

void AudioManager::Update(float /*dt*/) {
    mFMOD.Update();
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

void AudioManager::PlayBGM(const char* eventPath) {
    // 기존 BGM 정리
    if (mBGM) {
        mBGM->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        mBGM->release();
        mBGM = nullptr;
    }
    mBGM = mFMOD.CreateInstance(eventPath);
    // 필요 시 파라미터/버스 라우팅 세팅
    FMOD_CHECK(mBGM->start());
    // 유지하며 제어할 것이므로 release는 여기서 하지 않음(Shutdown/StopBGM에서)
}

void AudioManager::StopBGM() {
    if (!mBGM) return;
    mBGM->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    mBGM->release();
    mBGM = nullptr;
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
}

void AudioManager::SetBGMParam(const char* name, float value, bool ignoreSeekSpeed) {
    if (!mBGM) return; // 아직 BGM이 시작되지 않았다면 무시
    FMOD_CHECK(mBGM->setParameterByName(name, value, ignoreSeekSpeed));
}

void AudioManager::SetBGMParamLabel(const char* name, const char* label, bool ignoreSeekSpeed) {
    if (!mBGM) return;
    // 라벨형(Discrete Labeled) 파라미터를 문자열 라벨로 직접 설정
    FMOD_CHECK(mBGM->setParameterByNameWithLabel(name, label, ignoreSeekSpeed));
}

