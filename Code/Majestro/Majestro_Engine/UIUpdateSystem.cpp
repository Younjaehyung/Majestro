#include "pch.h"
#include "UIUpdateSystem.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "UITextComponent.h"
#include "HealthComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "CircularVisualizerComponent.h"
#include "AudioManager.h"
#include "CameraComponent.h"
#include "GameEvents.h"

UITransformSystem::UITransformSystem(World* world) : System::System(world)
{
}

void UITransformSystem::Initialize()
{
}

void UITransformSystem::Update(float dt)
{
    if (mWorld->HasComponentPool<UITransformComponent>() == false) return;

    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITransformComponent>() };

	WindowInfo window = RENDERMANAGER.GetWindow();

    Vec2 screenSize={(float) window.Width ,(float)window.Height };
    for (auto& e : entitys)
    {
        auto tr = mWorld->GetComponent<UITransformComponent>(e);
       
		
        Vec2 anchorBase = CalculateAnchor(tr->mAnchor, screenSize);
        tr->mFinalPixelPos = anchorBase + tr->mPosition;
		tr->mFinalSize = tr->mSize;
    }
}

Vec2 UITransformSystem::CalculateAnchor(Anchor anchor, const Vec2& screen)
{
    switch (anchor)
    {
    case Anchor::TopLeft:     return { 0, 0 };
    case Anchor::TopRight:    return { screen.x, 0 };
    case Anchor::BottomLeft:  return { 0, screen.y };
    case Anchor::BottomRight: return { screen.x, screen.y };
    case Anchor::Center:      return { screen.x * 0.5f, screen.y * 0.5f };
    }
    return { 0, 0 };
}


UIUpdateSystem::UIUpdateSystem(World* world) : System::System(world)
{
}

void UIUpdateSystem::Initialize()
{
}

void UIUpdateSystem::Update(float dt)
{
   UpdateScripts(dt);
   UpdateSpriteAnimation(dt);
   UpdateAudioVisualizer(dt);
   UpdateActiveUIEntities(dt);
   UpdateHpBarUI();
   UpdateTextContext(dt);
}

void UIUpdateSystem::UpdateScripts(float dt)
{
    if (!mWorld->HasComponentPool<UIScriptComponent>()) return;
    for (auto e : mWorld->GetEntitiesWithComponent<UIScriptComponent>())
    {
        auto* sc = mWorld->GetComponent<UIScriptComponent>(e);
        if (sc->mOnUpdate) sc->mOnUpdate(dt);
    }
}

void UIUpdateSystem::UpdateSpriteAnimation(float dt)
{
    if (false == mWorld->HasComponentPool<UISpriteComponent>())return;
    
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UISpriteComponent>() };

    WindowInfo window = RENDERMANAGER.GetWindow();

    Vec2 screenSize = { (float)window.Width ,(float)window.Height };
    for (auto& e : entitys)
    {
        UISpriteComponent* sp = mWorld->GetComponent<UISpriteComponent>(e);
        if (false == sp->mIsAnimated)
            continue;
        if (sp->mAnimationLoopTime <= 0.f)
            continue;

		sp->mAnimationUpdateTime += dt;

        sp->mAnimationUpdateTime = std::fmod(sp->mAnimationUpdateTime * sp->mAnimationSpeed, sp->mAnimationLoopTime);

        if (!sp->mTextures.empty())
        {
            const float progress = sp->mAnimationUpdateTime / sp->mAnimationLoopTime;
            const size_t frameIndex = static_cast<size_t>(progress * sp->mTextures.size()) % sp->mTextures.size();
            sp->mTexture = sp->mTextures[frameIndex];
        }
        else if (sp->mFrameCount > 1)
        {
            const float progress = sp->mAnimationUpdateTime / sp->mAnimationLoopTime;
            const int frameIndex = static_cast<int>(progress * sp->mFrameCount) % sp->mFrameCount;
            sp->SetCurrentFrame(frameIndex);
        }

    }

}


void UIUpdateSystem::EnsureHpBarUIEntities(UIHpBarComponent* hpBar)
{
    if (!hpBar)
        return;

    shared_ptr<Texture> hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mBackgroundMaterialName);
    shared_ptr<Texture> hpBarFillTexture = RESOURCEMANAGER.Get<Texture>(hpBar->mFillMaterialName);
    if (!hpBarBackgroundTexture)
        hpBarBackgroundTexture = RESOURCEMANAGER.Get<Texture>(L"HPBAR");
    if (!hpBarFillTexture)
        hpBarFillTexture = hpBarBackgroundTexture;

    if (!hpBarBackgroundTexture || !hpBarFillTexture)
        return;

    if (hpBar->mBackgroundUIEntity == NULL_ENTITY)
    {
        Entity background = mWorld->CreateEntity();
        auto& tr = mWorld->AddComponent<UITransformComponent>(background);
        tr.mAnchor = Anchor::TopLeft;
        tr.mPivot = Vec2(0.f, 0.f);
        tr.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        tr.mUILayerIndex = 10;

        mWorld->AddComponent<UISpriteComponent>(background, hpBarBackgroundTexture);
        hpBar->mBackgroundUIEntity = background;
    }
    else
    {
        UISpriteComponent* bgSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (bgSprite)
            bgSprite->mTexture = hpBarBackgroundTexture;
    }

    if (hpBar->mFillUIEntity == NULL_ENTITY)
    {
        Entity fill = mWorld->CreateEntity();
        auto& tr = mWorld->AddComponent<UITransformComponent>(fill);
        tr.mAnchor = Anchor::TopLeft;
        tr.mPivot = Vec2(0.f, 0.f);
        tr.mSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);
        tr.mUILayerIndex = 11;

        mWorld->AddComponent<UISpriteComponent>(fill, hpBarFillTexture);
        hpBar->mFillUIEntity = fill;
    }
    else
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite)
            fillSprite->mTexture = hpBarFillTexture;
    }
}

void UIUpdateSystem::SetHpBarVisibility(UIHpBarComponent* hpBar, bool visible)
{
    if (!hpBar)
        return;

    if (hpBar->mBackgroundUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* bgSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mBackgroundUIEntity);
        if (bgSprite)
            bgSprite->mVisible = visible;
    }

    if (hpBar->mFillUIEntity != NULL_ENTITY)
    {
        UISpriteComponent* fillSprite = mWorld->GetComponent<UISpriteComponent>(hpBar->mFillUIEntity);
        if (fillSprite)
            fillSprite->mVisible = visible;

        UICusSpriteComponent* fillCusSprite = mWorld->GetComponent<UICusSpriteComponent>(hpBar->mFillUIEntity);
        if (fillCusSprite)
            fillCusSprite->mVisible = visible;
    }
}

void UIUpdateSystem::UpdateTextContext(float dt)
{
    if (!mWorld->HasComponentPool<UITextComponent>())
         return;
     std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITextComponent>() };
     for (auto& e : entitys)
     {
         UITextComponent* text = mWorld->GetComponent<UITextComponent>(e);
         if (!text || !text->mOnTextChanged )
             continue;

         // 이벤트 소비형 콜백은 매 프레임 호출 (mIsDirty 무관)
         text->mOnTextChanged();
         text->mIsDirty = false;
	 }
}

void UIUpdateSystem::UpdateHpBarUI()
{
    if (!mWorld->HasComponentPool<UIHpBarComponent>() ||
        !mWorld->HasComponentPool<HealthComponent>() ||
        !mWorld->HasComponentPool<TransformComponent>())
    {
        return;
    }

    bool hasCamera = false;
    Matrix viewProj{};
    WindowInfo window = RENDERMANAGER.GetWindow();

    if (mWorld->HasComponentPool<MainCameraComponent>() && mWorld->HasComponentPool<CameraComponent>())
    {
        std::vector<Entity> cameras{ mWorld->GetEntitiesWithComponents<MainCameraComponent, CameraComponent>() };
        if (!cameras.empty())
        {
            CameraComponent* cam = mWorld->GetComponent<CameraComponent>(cameras[0]);
            if (cam)
            {
                viewProj = cam->mView * cam->mProjection;
                hasCamera = true;
            }
        }
    }

    std::vector<Entity> owners{ mWorld->GetEntitiesWithComponents<UIHpBarComponent, HealthComponent, TransformComponent>() };
    for (Entity owner : owners)
    {
        UIHpBarComponent* hpBar = mWorld->GetComponent<UIHpBarComponent>(owner);
        if (!hpBar)
            continue;

        if (hpBar->mTargetEntity == NULL_ENTITY)
            hpBar->mTargetEntity = owner;

        EnsureHpBarUIEntities(hpBar);

        TransformComponent* followTransform = mWorld->GetComponent<TransformComponent>(hpBar->mTargetEntity);
        HealthComponent* followHealth = mWorld->GetComponent<HealthComponent>(hpBar->mTargetEntity);

        UITransformComponent* bgTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mBackgroundUIEntity);
        UITransformComponent* fillTransform = mWorld->GetComponent<UITransformComponent>(hpBar->mFillUIEntity);
        if (!hasCamera || !followTransform || !followHealth || !bgTransform || !fillTransform)
        {
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const Vec3 worldPos = followTransform->mLocalPosition + hpBar->mWorldOffset;
        const Vec3 ndc = Vec3::Transform(worldPos, viewProj);

        if (ndc.z < 0.0f || ndc.z > 1.0f)
        {
            SetHpBarVisibility(hpBar, false);
            continue;
        }

        const float pixelX = (ndc.x * 0.5f + 0.5f) * static_cast<float>(window.Width);
        const float pixelY = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(window.Height);

        bgTransform->mAnchor = Anchor::TopLeft;
        bgTransform->mPosition = Vec2(pixelX - hpBar->mMaxWidth * 0.5f, pixelY);
        bgTransform->mFinalSize = Vec2(hpBar->mMaxWidth, hpBar->mHeight);

        fillTransform->mAnchor = Anchor::TopLeft;
        fillTransform->mPosition = bgTransform->mPosition;
        const float followRatio = std::clamp(static_cast<float>(followHealth->mCurrentHp) / static_cast<float>((std::max)(1, followHealth->mMaxHp)), 0.0f, 1.0f);
        fillTransform->mFinalSize = Vec2(hpBar->mMaxWidth * followRatio, hpBar->mHeight);

        SetHpBarVisibility(hpBar, true);
    }
}

void UIUpdateSystem::UpdateAudioVisualizer(float dt)
{
    if (!mWorld->HasComponentPool<CircularVisualizerComponent>())
        return;

    // FMOD FFT DSP에서 스펙트럼 데이터 폴링
    std::vector<float> spectrum;
    if (!AUDIOMANAGER.GetSpectrumData(spectrum))
        return;

    int specSize = static_cast<int>(spectrum.size());
    if (specSize == 0)
        return;

    float sampleRate = AUDIOMANAGER.GetSpectrumSampleRate();

    // 스파이크 위치 랜덤 생성용
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> countDist(2, 4);
    std::uniform_int_distribution<int> idxDist(0, CIRC_VIS_POINTS - 1);

    for (auto entity : mWorld->GetEntitiesWithComponent<CircularVisualizerComponent>())
    {
        auto* vis = mWorld->GetComponent<CircularVisualizerComponent>(entity);
        if (!vis || !vis->isVisible)
            continue;

        // ── 1. 스펙트럼 → 32개 내부 대역 처리 ────────────────────────────
        // 128개 포인트 직접 분할 시 각 대역이 너무 좁아 peak 값이 거의 0이 됨.
        // AudioVisualizerSystem과 동일하게 32개 대역을 사용해 충분한 에너지를 확보한 후
        // 128개 포인트로 선형 보간 업샘플링한다.
        float bands[kInternalBands] = {};
        for (int band = 0; band < kInternalBands; band++)
        {
            auto [b0, b1] = GetBinRange(band, kInternalBands, specSize, sampleRate);

            float peak = 0.f;
            for (int b = b0; b < b1; b++)
                peak = max(peak, spectrum[b]);

            // AudioVisualizerSystem과 동일한 EQ gain 적용
            float freqT = static_cast<float>(band) / static_cast<float>(kInternalBands - 1);
            float eqGain = 0.5f + freqT * 3.5f;
            bands[band] = std::clamp(peak * vis->gain * eqGain, 0.6f, 1.f);
        }

        // ── 2. 32개 → 128개 선형 보간 업샘플링 후 스무딩 적용 ───────────
        float bassEnergy = 0.f;
        for (int i = 0; i < CIRC_VIS_POINTS; i++)
        {
            // i를 kInternalBands 범위로 매핑
            float t = static_cast<float>(i) / static_cast<float>(CIRC_VIS_POINTS - 1)
                * static_cast<float>(kInternalBands - 1);
            int   lo = static_cast<int>(t);
            int   hi = min(lo + 1, kInternalBands - 1);
            float frac = t - static_cast<float>(lo);

            float target = bands[lo] * (1.f - frac) + bands[hi] * frac;
            float& cur = vis->waveAmplitudes[i];

            // 비대칭 스무딩: 상승 빠르게(거친 느낌), 하강 천천히
            float speed = (target > cur) ? vis->riseSmooth : vis->fallSmooth;
            cur += (target - cur) * speed * dt;
            cur = std::clamp(cur, 0.f, 1.f);

            if (i < kBassPoints)
                bassEnergy += cur;
        }
        bassEnergy /= static_cast<float>(kBassPoints);

        // ── 3. 스파이크 타이머 갱신 ────────────────────────────────────────
        vis->cooldownTimer -= dt;
        for (auto& sp : vis->spikes)
        {
            if (sp.pointIdx < 0)
                continue;

            sp.timer -= dt;
            if (sp.timer <= 0.f)
            {
                sp.pointIdx = -1;
                sp.strength = 0.f;
            }
            else
            {
                // 남은 시간 비율로 선형 감쇠
                sp.strength = sp.timer / vis->spikeDuration;
            }
        }

        // ── 4. 스파이크 발생 조건 판단 ────────────────────────────────────
        if (bassEnergy >= vis->spikeThreshold && vis->cooldownTimer <= 0.f)
        {
            vis->cooldownTimer = vis->spikeCooldown;

            int count = countDist(rng);  // 2~4개 랜덤
            int slot = 0;
            for (auto& sp : vis->spikes)
            {
                if (slot >= count)
                    break;
                sp.pointIdx = idxDist(rng);
                sp.strength = 1.f;
                sp.timer = vis->spikeDuration;
                slot++;
            }
        }

        // ── 5. 스파이크 진폭 적용 ─────────────────────────────────────────
        // waveAmplitudes > 1.0 을 허용하여 스파이크를 표현.
        // Pass에서 r = baseRadius + amp * maxAmplitude 로 계산되므로
        // amp = spikeMultiplier (예: 4.5) 이면 반지름이 일반 파동보다 4.5배 돌출된다.
        for (const auto& sp : vis->spikes)
        {
            if (sp.pointIdx < 0)
                continue;

            float& amp = vis->waveAmplitudes[sp.pointIdx];
            float  spikeAmp = sp.strength * vis->spikeMultiplier;  // > 1 허용
            amp = max(amp, spikeAmp);
        }
    }
}


void UIUpdateSystem::UpdateActiveUIEntities(float dt)
{
    if (false == mWorld->HasComponentPool<UIActionComponent>())return;

    // 이번 프레임에 박자 이벤트가 왔는지 확인
    bool beatFired = false;
    mWorld->GetEventManager()->Consume<EvBeat>([&](const EvBeat&) {
        beatFired = true;
    });

    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UIActionComponent>() };
    for (auto& e : entitys)
    {
        UIActionComponent* uiAction = mWorld->GetComponent<UIActionComponent>(e);
        UITransformComponent* uiTrans = mWorld->GetComponent<UITransformComponent>(e);

        // 박자 이벤트 수신 시 Bounce 애니메이션 리셋
        if (beatFired && uiAction->mState == UIActionState::Bounce)
        {
            uiAction->mElapsedTime = 0.f;
            uiAction->mIsActive = true;
        }

        if (false == uiAction->mIsActive) continue;
		uiAction->mElapsedTime += dt;

        if (uiAction->mOnCustomAction)
        {
            uiAction->mOnCustomAction();
            continue;
        }

        if (uiAction->mState == UIActionState::Vibration) {
            if (uiAction->mOnCustomAction)
            {
                uiAction->mOnCustomAction();
            }
            else {
                uiTrans->mFinalPixelPos += Vec2(
                    std::sin(uiAction->mElapsedTime * uiAction->mVibrationFrequency * 2.f * 3.14159f) * uiAction->mVibrationAmplitude,
                    std::cos(uiAction->mElapsedTime * uiAction->mVibrationFrequency * 2.f * 3.14159f) * uiAction->mVibrationAmplitude
                );
            }
        }
        else if (uiAction->mState == UIActionState::Hovered) {
            float progress = std::clamp(uiAction->mElapsedTime / uiAction->mDuration, 0.f, 1.f);
            uiTrans->mFinalSize = uiTrans->mSize * (uiAction->mDefaultScale + (uiAction->mHoverScale - uiAction->mDefaultScale) * progress);
		}
        else if (uiAction->mState == UIActionState::Bounce) {
            // 박자에 맞춰 한 번 튕기는 애니메이션 (mDuration: 튕김 지속 시간)
            float progress = std::clamp(uiAction->mElapsedTime / uiAction->mDuration, 0.f, 1.f);
            float bounce = std::sin(progress * 3.14159f) * uiAction->mBounceAmplitude;
            uiTrans->mFinalSize = uiTrans->mSize * (1.f + bounce);
		}




        
        
        
        
        
        if (uiAction->mElapsedTime >= uiAction->mDuration) {
            
            if(!uiAction->mIsLoop)  uiAction->mIsActive = false;

            uiAction->mElapsedTime = 0.f;
		}

    }
}

std::pair<int, int> UIUpdateSystem::GetBinRange(
    int pointIdx, int totalPoints, int spectrumSize, float sampleRate) const
{
    // 로그 스케일: 20Hz ~ 16kHz 를 CIRC_VIS_POINTS개 대역으로 분할
    constexpr float kMinHz = 20.f;
    constexpr float kMaxHz = 16000.f;
    const float logMin = std::log2f(kMinHz);
    const float logMax = std::log2f(kMaxHz);

    float t0 = static_cast<float>(pointIdx) / static_cast<float>(totalPoints);
    float t1 = static_cast<float>(pointIdx + 1) / static_cast<float>(totalPoints);

    float hz0 = std::pow(2.f, logMin + t0 * (logMax - logMin));
    float hz1 = std::pow(2.f, logMin + t1 * (logMax - logMin));

    // Hz → FFT 빈 인덱스 (FFT 빈 해상도 = sampleRate / (2 * spectrumSize))
    float hzPerBin = (sampleRate * 0.5f) / static_cast<float>(spectrumSize);
    int b0 = static_cast<int>(hz0 / hzPerBin);
    int b1 = static_cast<int>(hz1 / hzPerBin);

    b0 = std::clamp(b0, 0, spectrumSize - 1);
    b1 = std::clamp(b1 + 1, b0 + 1, spectrumSize);
    return { b0, b1 };
}
