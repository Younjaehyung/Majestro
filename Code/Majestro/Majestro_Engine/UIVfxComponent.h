#pragma once
#include "Component.h"
#include "Vfx.h"

// UI 레이어에서 재생되는 Effekseer 이펙트 컴포넌트
// 직교 투영(스크린 픽셀 좌표)으로 렌더링됨
class UIVfxComponent : public Component<UIVfxComponent>
{
public:
	UIVfxComponent() = default;

	void SetScreenPosition(float x, float y, float z = 0.f)
	{
		mScreenX = x;
		mScreenY = y;
		mScreenZ = z;
	}

public:
	shared_ptr<Vfx>   mVfx       = nullptr;
	Effekseer::Handle efkHandle  = -1;

	float mTotalTime = 0.f;
	bool  mIsPlaying = false;
	bool  mIsPaused  = false;
	bool  mIsLoop    = false;
	float mScale     = 1.f;

	// 스크린 픽셀 좌표 (0,0 = 좌상단)
	float mScreenX = 0.f;
	float mScreenY = 0.f;
	float mScreenZ = 0.5f;  // near plane(0) 클리핑 방지
};
