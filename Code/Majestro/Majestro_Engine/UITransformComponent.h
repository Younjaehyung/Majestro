#pragma once
#include "Component.h"


enum class Anchor
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center
};

class UITransformComponent : public Component<UITransformComponent>
{
public: // 통일된 UI 위치


    Anchor mAnchor = Anchor::TopLeft;

    uint8 mUILayerIndex = 0;

    Vec2 mPosition;     // anchor 기준 오프셋 (pixel)
    Vec2 mSize;         // pixel
    Vec2 mPivot;        // (0~1)
    Vec2 mScale = { 1.f, 1.f }; // mFinalSize = mSize * mScale (연출용)

    Vec2 mFinalPixelPos;   // 최종 화면 픽셀 좌표
	Vec2 mFinalSize;       // 최종 화면 픽셀 크기
};
//
//좌상단 정렬 UI를 원함(피벗 0, 0)
//inst.Pivot = (0, 0)
//inst.Position = (100, 50)   [ UI의 좌상단이(100, 50)에 옴
//inst.Size = (256, 64)       [ == 256x64 px
//
//중앙 정렬 UI를 원함(피벗 0.5, 0.5)
//inst.Pivot = (0.5, 0.5)
//inst.Position = (960, 540)  [UI의 중앙이 화면 중앙에 옴(FullHD 기준)
//inst.Size = (256, 64)
//
//우하단 정렬(피벗 1, 1)
//inst.Pivot = (1, 1)
//inst.Position = (ScreenW - 20, ScreenH - 20) [ UI의 우하단 꼭짓점이 거기에 옴
//inst.Size = (256, 64)
//


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

// 생동감 있는 UI 효과를 위한 컴포넌트
class UIActionComponent : public Component<UIActionComponent>
{
public:
	UIActionState mState = UIActionState::None;
	UIActor mActor = UIActor::None;

	std::function<void()> mOnCustomAction; // 커스텀 액션 콜백 (예: 클릭 시 실행할 함수)

public:

	// Hover 효과를 위한 변수
	float mHoverScale = 1.2f; // 호버 시 크기 배율
    float mCurrentScale = 1.0f; // 현재 크기 배율
	float mDefaultScale = 0.0f; // 기본 크기 배율

	// Bounce 애니메이션을 위한 변수
	bool mIsLoop = false; // 애니메이션 무한 반복 여부
	float mDuration = 0.2f; // 애니메이션 지속 시간 (초)
	float mElapsedTime = 0.f; // 애니메이션 경과 시간
	float mBounceAmplitude = 0.05f; // Bounce 애니메이션 크기 변화 정도 (배율)
	float mBounceFrequency = 5.f; // Bounce 애니메이션 빈도 (Hz)
	float mBounceDamping = 5.f; // Bounce 애니메이션 감쇠 정도 (높을수록 빨리 진동이 줄어듦)
	// Vibration 효과를 위한 변수
	float mVibrationAmplitude = 5.f; // 진동 시 위치 흔들림 정도 (픽셀)
	float mVibrationFrequency = 20.f; // 진동 시 흔들림 빈도 (Hz)
public:
	bool mIsActive = true; // 현재 액션이 활성화되어 있는지 여부

};