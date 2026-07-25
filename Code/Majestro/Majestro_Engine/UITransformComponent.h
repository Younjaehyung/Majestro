#pragma once
#include "Component.h"

enum class Anchor
{
	TopLeft,
	TopCenter,
	TopRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
	Center
};

enum class UILayoutMode
{
	Pixel,
	ScreenRatio,
	ReferenceResolution
};

class UITransformComponent : public Component<UITransformComponent>
{
public:
	Anchor mAnchor = Anchor::TopLeft;
	UILayoutMode mLayoutMode = UILayoutMode::ReferenceResolution;
	Vec2 mReferenceResolution = Vec2(2560.f, 1440.f); // 1560x1440 기준

	uint8 mUILayerIndex = 0;

	Vec2 mPosition;      // Pixel 모드: 현재 화면 픽셀. ReferenceResolution 모드: 기준 해상도 픽셀
	Vec2 mSize;          // Pixel 모드: 현재 화면 픽셀. ReferenceResolution 모드: 기준 해상도 픽셀
	Vec2 mPositionRatio; // ScreenRatio 모드: anchor 기준 오프셋 비율. x는 화면 너비, y는 화면 높이 기준
	Vec2 mSizeRatio;     // ScreenRatio 모드: 크기 비율. x는 화면 너비, y는 화면 높이 기준
	Vec2 mPivot;         // (0~1)
	Vec2 mScale = { 1.f, 1.f };

	Vec2 mFinalPixelPos; // 최종 화면 픽셀 좌표
	Vec2 mFinalSize;     // 최종 화면 픽셀 크기

	void UseScreenRatioLayout(const Vec2& positionRatio, const Vec2& sizeRatio)
	{
		mLayoutMode = UILayoutMode::ScreenRatio;
		mPositionRatio = positionRatio;
		mSizeRatio = sizeRatio;
	}

	void UseReferenceResolutionLayout(const Vec2& referenceResolution = Vec2(2560.f, 1440.f))
	{
		mLayoutMode = UILayoutMode::ReferenceResolution;
		mReferenceResolution = referenceResolution;
	}
};

enum class UIActor
{
	None,
	Player, // 플레이어 캐릭터 관련 UI
	SoundEffect, // 사운드로 인한 UI 효과
	GamePlay, // 게임룰 UI, 스코어, 타이머 등
};

enum class UIActionState
{
	None,
	Hovered,
	Vibration,
	Bounce,
};


inline constexpr float kUIBeatBounceAmplitude = 0.05f;	// HUD 박자 바운스 기본 세기

class UIActionComponent : public Component<UIActionComponent>
{
public:
	UIActionState mState = UIActionState::None;
	UIActor mActor = UIActor::None;

	std::function<void()> mOnCustomAction; // 커스텀 액션 콜백

	// Hover 효과를 위한 변수
	float mHoverScale = 1.2f; // 호버 시 크기 배율
	float mCurrentScale = 1.0f; // 현재 크기 배율
	float mDefaultScale = 0.0f; // 기본 크기 배율

	// Bounce 애니메이션을 위한 변수
	bool mIsLoop = false; // 애니메이션 무한 반복 여부
	float mDuration = 0.2f; // 애니메이션 지속 시간
	float mElapsedTime = 0.f; // 애니메이션 경과 시간
	float mBounceAmplitude = kUIBeatBounceAmplitude; // Bounce 크기 변화 정도
	float mBounceFrequency = 5.f; // Bounce 빈도
	float mBounceDamping = 5.f; // Bounce 감쇠 정도

	// Vibration 효과를 위한 변수
	float mVibrationAmplitude = 5.f; // 진동 위치 흔들림 정도
	float mVibrationFrequency = 20.f; // 진동 빈도

	bool mIsActive = true; // 현재 액션 활성화 여부
};
