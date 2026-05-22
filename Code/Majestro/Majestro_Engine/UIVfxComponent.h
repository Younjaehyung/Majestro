#pragma once
#include "Component.h"
#include "Vfx.h"

// UI 레이어에서 재생되는 Effekseer 이펙트 컴포넌트
// 직교 투영(스크린 픽셀 좌표)으로 렌더링됨
// 위치는 반드시 같은 엔티티의 UITransformComponent.mFinalPixelPos에서 읽음
class UIVfxComponent : public Component<UIVfxComponent>
{
public:
	UIVfxComponent() = default;
	UIVfxComponent(shared_ptr<Vfx> vfx, bool loop = false, float scale = 1.f)
		: mVfx(vfx), mIsLoop(loop), mScale(scale) {
	}
public:
	shared_ptr<Vfx>   mVfx       = nullptr;
	Effekseer::Handle efkHandle  = -1;

	float mTotalTime = 10.f;
	bool  mIsPlaying = false;
	bool  mIsPaused  = false;
	bool  mIsLoop    = false;
	float mScale     = 1.f;

	float mScreenZ = 0.0f;  // near plane(0) 클리핑 방지

	bool  mVisible   = true; // false 이면 UIEffectSystem 이 stop/스킵
};
