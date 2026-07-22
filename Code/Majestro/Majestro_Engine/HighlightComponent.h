#pragma once
#include "Component.h"

class HighlightComponent : public Component<HighlightComponent>
{
public:
    HighlightComponent() = default;


    Vec3  mColor{ 0.45f, 0.9f, 1.8f };  // 강조 색 (나노강화 청록/파랑, HDR 가산이라 1 초과 = 발광 강화)
    float mBaseIntensity{ 2.0f };       // 최대 강조 강도
    float mPulseSpeed{ 9.0f };          // 펄스 속도
    float mPulseDepth{ 0.55f };         // 펄스 깊이

   
    float mFadeIn{ 0.15f };             // 켜질 때 페이드인 시간
    float mFadeOut{ 0.40f };            // 꺼질 때 페이드아웃 시간

    // 상승 오라/기둥 VFX
    const wchar_t* mSparkVfx{ L"VFX_Player_Rebirth" }; // vfx
    Vec3  mSparkOffset{ 0.f, 20.f, 0.f };              // 캐릭터 기준 스폰/추종 오프셋
    Vec3  mSparkScale{ 100.f, 100.f, 100.f };          // 이펙트 스케일

    // 지면 충격파 데칼
    const wchar_t* mGroundDecalTex{ L"UI_VFXDecal_Sheet" }; // 데칼 
    int   mGroundDecalCols{ 4 };
    int   mGroundDecalRows{ 1 };
    int   mGroundDecalIndex{ 0 };                      // 0 = 지면 균열 셀
    float mGroundDecalRadius{ 320.f };
    Vec4  mGroundDecalColor{ 0.5f, 1.4f, 2.4f, 1.f }; 
    float mGroundDecalLifetime{ 0.9f };

    // 카메라 (로컬만)
    Vec3  mCamShakeAngles{ 2.6f, 1.8f, 0.0f };         // 흔들림 강화 (pitch/yaw)
	float mCamShakeDuration{ 0.45f };                  // 흔들림 지속시간
	float mCamShakeFrequency{ 26.f };                  // 흔들림 주파수
	float mCamDollyDistance{ 130.f };                  // 카메라 돌리기 거리
	float mCamDollyHold{ 0.30f };                      // 카메라 돌리기 유지시간
	float mCamDollyInSpeed{ 22.f };                    // 카메라 돌리기 속도 (초당 거리)
	float mCamDollyOutSpeed{ 2.5f };                   // 카메라 복귀 속도 (초당 거리)

    // 오디오 (현재 로컬임)
    const char* mBurstAudioEvent{ nullptr };           // "event:/SFX/Ultimate"

    // 런타임
    float mRemainSec{ 0.f };            // 남은 지속시간 (0 이하면 페이드아웃)
    float mFadeGate{ 0.f };             // 페이드 게이트 (0~1)
    float mElapsed{ 0.f };              // 맥동 위상용 누적 시간
    bool  mSparkFired{ false };         // 이번 발동에서 스파크를 이미 터뜨렸는지 (중복 방지)


    float mCurrentIntensity{ 0.f };     // 최종 강도
    float mBurstGate{ 1.8f };           // 번쩍임 세기

    // 지정한 시간만큼 강조
    void Trigger(float durationSec)
    {
        mRemainSec = durationSec;
        mFadeGate  = mBurstGate;
        mSparkFired = false;   // 새 발동
    }
};
