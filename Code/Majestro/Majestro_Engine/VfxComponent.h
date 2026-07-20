#pragma once
#include "Component.h"
#include "Entity.h"
#include "Vfx.h"

class VfxComponent : public Component<VfxComponent>
{
	// Effekseer 이펙트 한 개의 재생 상태를 담는 컴포넌트 (사실상 root)
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
		mStopRootWhenDisabled = false;
		mShouldPlay = false;
		mInUse = false;
		mFinished = false;
		mScale = Vec3::Zero;
		mUseWorldMatrix = false;
		mWorldMatrix = Matrix::Identity;
		mRootStopped = false;
		mFollowTarget = Entity{};
		mFollowOffset = Vec3::Zero;
		mFollowRotation = false;
		mFollowInitialRotation = Vec3::Zero;
		mDebugCylinderLength = 0.0f;
		mDebugCylinderRadius = 0.0f;
		mStopWhenCasterLeavesSpecial = false;
		mFollowForwardOffset = 0.0f;
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
		mStopRootWhenDisabled = false;
		mShouldPlay = (mVfx != nullptr);
		mInUse = true;
		mFinished = false;
		mScale = scale;
		mUseWorldMatrix = false;
		mWorldMatrix = Matrix::Identity;
		mRootStopped = false;
		mFollowTarget = Entity{};
		mFollowOffset = Vec3::Zero;
		mFollowRotation = false;
		mFollowInitialRotation = Vec3::Zero;
		mDebugCylinderLength = 0.0f;
		mDebugCylinderRadius = 0.0f;
		mStopWhenCasterLeavesSpecial = false;
		mFollowForwardOffset = 0.0f;
	}

	shared_ptr<Vfx> mVfx = nullptr;				// 재생할 이펙트 리소스(.efk 래퍼)
	Effekseer::Handle efkHandle = -1;			// Effekseer 런타임이 발급한 인스턴스 핸들

	// 재생 상태
	float	mTotalTime = 0.f;				// 현재 재생이 시작된 뒤 누적 경과 시간
	bool	mIsPlaying = false;
	bool	mIsPaused  = false;
	bool	mIsLoop    = false;				// 반복 재생 이펙트인지

	bool	mRestartWhenFinished = false;	// 끝나면 자동으로 다시 시작할지
	bool	mShouldPlay = true;				// 재생 허용 여부 (Entity는 살아 있지만 VFX만 꺼야 하는 경우)
	bool	mFinished = false;				// 원샷 VFX가 종료되었음을 표시하는 flag


	// Socket VFX (소켓 추적용)
	bool	mStopRootWhenDisabled = false;		// emission을 끈 뒤에도 기존 트레일 파티클이 자연스럽게 사라지도록 StopRoot를 호출할지
	bool	mRootStopped = false;				// StopRoot 호출 후 true. 살아 있는 핸들이 소켓을 계속 따라가면서 옛 파티클은 페이드되게 함 
	bool	mUseWorldMatrix = false;			// true면 Euler 각도로 회전을 재구성하지 않고, 최종 소켓 행렬을 그대로 Effekseer에 전달 
	Matrix	mWorldMatrix = Matrix::Identity;	// EffectPass::SetMatrix가 사용하는 최종 변환 행렬
	

	// 오브젝트 풀 관리
	bool	mIsPooled = false;				// VFX 풀에서 관리되는 컴포넌트인지 표시
	bool	mInUse = false;					// 풀 Entity가 현재 사용 중인지 표시
	bool	mAutoReturn = false;			// 재생이 끝났을 때 자동으로 풀에 반환할지 여부
	
	// 추종(Follow) — 유효한 대상이면 VfxSystem이 매 프레임 대상 위치로 transform을 갱신한다.
	Entity	mFollowTarget;					// 추종할 엔티티 (유효하지 않으면 고정 월드 VFX)
	Vec3	mFollowOffset = Vec3::Zero;		// 추종 대상 월드 위치 기준 오프셋
	bool	mFollowRotation = false;			// 시전자의 조준 회전도 매 프레임 추종
	Vec3	mFollowInitialRotation = Vec3::Zero;	// 원격 플레이어의 피치 보존용 최초 회전
	float	mDebugCylinderLength = 0.0f;		// Player Attack Ranges에서 표시할 직선 판정 길이
	float	mDebugCylinderRadius = 0.0f;		// Player Attack Ranges에서 표시할 원통 반경
	bool	mStopWhenCasterLeavesSpecial = false;
	float	mFollowForwardOffset = 0.0f;

	// 트랜스폼
	Vec3	mScale     = Vec3(1.f, 1.f, 1.f);
	Vec3	mAttachOffset = Vec3::Zero;		// 부착 대상 로컬 오프셋. (0 : 미적용)
	::Effekseer::Vector3D mPosition = ::Effekseer::Vector3D(0.0f, 0.0f, 0.0f);
	

};
