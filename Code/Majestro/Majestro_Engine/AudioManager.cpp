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

	// 모든 mAllBGM을 nullptr로 초기화


}

void AudioManager::Shutdown() {

    if (!mAllBGM.empty()) {
        for (auto& bgm : mAllBGM) {
            if (!bgm) {
                continue;
            }
            bgm->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
            bgm->release();
            bgm = nullptr;
        }
        mAllBGM.clear();
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
        mAllBGM[idx]->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
        mAllBGM[idx]->release();
        mAllBGM[idx] = nullptr;
    }

    mBGM = mFMOD.CreateInstance(eventPath);
	mAllBGM[idx] = mBGM;
    // �ʿ� �� �Ķ����/���� ����� ����
    FMOD_CHECK(mBGM->start());
    // �����ϸ� ������ ���̹Ƿ� release�� ���⼭ ���� ����(Shutdown/StopBGM����)
}

void AudioManager::StopBGM(SOUNDNAME soundEnum) {
    uint32 idx = static_cast<uint32>(soundEnum);
    if (idx >= mAllBGM.size()) return;

    if (!mAllBGM[idx]) return;
    mAllBGM[idx]->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    mAllBGM[idx]->release();
    mAllBGM[idx] = nullptr;
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

