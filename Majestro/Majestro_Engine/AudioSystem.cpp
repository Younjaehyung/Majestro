#include "pch.h"
#include "AudioSystem.h"

static std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    std::filesystem::path p = std::filesystem::path(a) / b;
    return p.u8string();
}

void AudioSystem::Initialize(const std::string& bankRoot, bool rightHanded3D) {
    mBankRoot = bankRoot;

    FMOD_CHECK(FMOD::Studio::System::create(&mStudio));

    // Studio 초기화 플래그
    FMOD_STUDIO_INITFLAGS studioFlags = FMOD_STUDIO_INIT_NORMAL;
    FMOD_INITFLAGS coreFlags = FMOD_INIT_NORMAL;
    if (rightHanded3D) {
        // FMOD은 기본 좌표계가 좌수계. 엔진이 RH면 플래그 사용.
        coreFlags = FMOD_INIT_3D_RIGHTHANDED | coreFlags;
    }

    // 버퍼/보이스 등은 프로젝트에 맞게 필요시 조정
    FMOD_CHECK(mStudio->initialize(1024, studioFlags, coreFlags, nullptr));

    // Core 시스템 가져오기
    FMOD_CHECK(mStudio->getCoreSystem(&mCore));

    // 샘플레이트/스피커모드(필요시): 48kHz / 7.1 예시
    // mCore->setSoftwareFormat(48000, FMOD_SPEAKERMODE_7POINT1, 0);

    // 3D 설정: dopplerScale, distanceFactor(m 단위), rolloffScale
    FMOD_CHECK(mCore->set3DSettings(1.0f, 1.0f, 1.0f));

    // 필수 뱅크(마스터/스트링)는 보통 여기서 선로드
    LoadBank("Master.bank");
    LoadBank("Master.strings.bank", FMOD_STUDIO_LOAD_BANK_NORMAL, false);
}

void AudioSystem::Shutdown() {
    // 뱅크 언로드
    for (auto& [name, bank] : mBanks) {
        if (bank) {
            bank->unloadSampleData();
            bank->unload();
        }
    }
    mBanks.clear();

    if (mStudio) {
        mStudio->unloadAll();
        mStudio->release();
        mStudio = nullptr;
        mCore = nullptr;
    }
}

void AudioSystem::Update(float) {
    // 매 프레임 호출
    FMOD_CHECK(mStudio->update());
}

void AudioSystem::SetListenerAttributes(const FMOD_3D_ATTRIBUTES& attr, int index) {
    FMOD_CHECK(mStudio->setListenerAttributes(index, &attr));
}

void AudioSystem::LoadBank(const std::string& bankName, FMOD_STUDIO_LOAD_BANK_FLAGS flags, bool preloadSampleData) {
    auto full = JoinPath(mBankRoot, bankName);
    if (mBanks.count(bankName)) return; // 이미 로드

    FMOD::Studio::Bank* bank = nullptr;
    FMOD_CHECK(mStudio->loadBankFile(full.c_str(), flags, &bank));
    mBanks[bankName] = bank;

    if (preloadSampleData) {
        FMOD_CHECK(bank->loadSampleData());
    }
}

void AudioSystem::UnloadBank(const std::string& bankName) {
    auto it = mBanks.find(bankName);
    if (it == mBanks.end()) return;
    FMOD::Studio::Bank* bank = it->second;
    if (bank) {
        bank->unloadSampleData();
        bank->unload();
    }
    mBanks.erase(it);
}

FMOD::Studio::EventInstance* AudioSystem::CreateInstance(const char* eventPath) {
    FMOD::Studio::EventDescription* desc = nullptr;
    FMOD_CHECK(mStudio->getEvent(eventPath, &desc));
    FMOD::Studio::EventInstance* inst = nullptr;
    FMOD_CHECK(desc->createInstance(&inst));
    return inst;
}

void AudioSystem::PlayOneShot(const char* eventPath) {
    auto* inst = CreateInstance(eventPath);
    FMOD_CHECK(inst->start());
    FMOD_CHECK(inst->release()); // 자동 수명관리
}

void AudioSystem::PlayOneShot3D(const char* eventPath, const FMOD_3D_ATTRIBUTES& attr) {
    auto* inst = CreateInstance(eventPath);
    FMOD_CHECK(inst->set3DAttributes(&attr));
    FMOD_CHECK(inst->start());
    FMOD_CHECK(inst->release());
}

void AudioSystem::ReleaseInstance(FMOD::Studio::EventInstance* inst) {
    if (inst) inst->release();
}

FMOD::Studio::Bus* AudioSystem::GetBus(const char* busPath) {
    FMOD::Studio::Bus* bus = nullptr;
    FMOD_CHECK(mStudio->getBus(busPath, &bus));
    return bus;
}

FMOD::Studio::VCA* AudioSystem::GetVCA(const char* vcaPath) {
    FMOD::Studio::VCA* vca = nullptr;
    FMOD_CHECK(mStudio->getVCA(vcaPath, &vca));
    return vca;
}
