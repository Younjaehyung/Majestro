#pragma once
#include "Component.h"
#include "Vfx.h"

class VfxComponent : public Component<VfxComponent>
{

public:
	VfxComponent() = default;

	void SetPosition(float x, float y, float z)
	{
		mPosition.X = x;
		mPosition.Y = y;
		mPosition.Z = z;
	}

	void ResetForPoolIdle()
	{
		// 원샷 VFX 풀 반환 시 이전 재생 정보가 다음 대여에 섞이지 않도록 상태를 초기화.
		mVfx = nullptr;
		efkHandle = -1;
		mTotalTime = 0.f;
		mIsPlaying = false;
		mIsPaused = false;
		mIsLoop = false;
		mRestartWhenFinished = false;
		mShouldPlay = false;
		mInUse = false;
		mFinished = false;
		mScale = Vec3::Zero;
		SetPosition(0.f, 0.f, 0.f);
	}

	void ResetForPoolPlay(shared_ptr<Vfx> vfx, const Vec3& scale, bool loop)
	{
		// 원샷 VFX 대여 시 EffectPass가 새 Effekseer handle을 다시 만들도록 재생 상태를 초기화.
		mVfx = vfx;
		efkHandle = -1;
		mTotalTime = 0.f;
		mIsPlaying = false;
		mIsPaused = false;
		mIsLoop = loop;
		mRestartWhenFinished = loop;
		mShouldPlay = (mVfx != nullptr);
		mInUse = true;
		mFinished = false;
		mScale = scale;
	}

	shared_ptr<Vfx> mVfx = nullptr;
	Effekseer::Handle efkHandle = -1;


	float	mTotalTime = 0.f;
	bool	mIsPlaying = false;
	bool	mIsPaused  = false;
	bool	mIsLoop    = false;
	bool	mRestartWhenFinished = false;	// 월드 마커 VFX는 true, 원샷 VFX는 false
	// 재사용되는 총알 엔티티가 비활성 상태일 때 VFX가 다시 자동 재생되지 않도록 재생 허용 여부
	bool	mShouldPlay = true;				// Entity는 살아 있지만 VFX만 꺼야 하는 경우
	bool	mIsPooled = false;				// VFX 풀에서 관리되는 컴포넌트인지 표시
	bool	mInUse = false;					// 풀 Entity가 현재 사용 중인지 표시
	bool	mAutoReturn = false;			// 재생이 끝났을 때 자동으로 풀에 반환할지 여부
	bool	mFinished = false;				// 원샷 VFX가 끝 flag
	Vec3	mScale     = Vec3(1.f, 1.f, 1.f);
	Vec3	mAttachOffset = Vec3::Zero;		// 부착 대상 로컬 오프셋. (0 : 미적용)
	::Effekseer::Vector3D mPosition = ::Effekseer::Vector3D(0.0f, 0.0f, 0.0f);

};

