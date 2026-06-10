#pragma once
#include "Component.h"
#include "PhysicsWorld.h"


enum class PROJECTION_TYPE
{
	PERSPECTIVE, // 원근 투영
	ORTHOGRAPHIC, // 직교 투영
};

enum PLANE_TYPE : uint8
{
	PLANE_FRONT,
	PLANE_BACK,
	PLANE_UP,
	PLANE_DOWN,
	PLANE_LEFT,
	PLANE_RIGHT,

	PLANE_END
};


struct CameraParams {
	Matrix MatView;
	Matrix MatProjection;
	Matrix MatViewInv;				// view의 역행렬
	Matrix MatProjectionInv;		// Projection의 역행렬	(사용은 선택)
};


struct Frustum {
	array<Vec4, PLANE_END> Planes;


	void FinalUpdate(Matrix S_MatView, Matrix S_MatProjection)
	{
		Matrix matViewInv = S_MatView.Invert();
		Matrix matProjectionInv = S_MatProjection.Invert();
		Matrix matInv = matProjectionInv * matViewInv;

		vector<Vec3> worldPos =
		{
			::XMVector3TransformCoord(Vec3(-1.f, 1.f, 0.f), matInv),
			::XMVector3TransformCoord(Vec3(1.f, 1.f, 0.f), matInv),
			::XMVector3TransformCoord(Vec3(1.f, -1.f, 0.f), matInv),
			::XMVector3TransformCoord(Vec3(-1.f, -1.f, 0.f), matInv),
			::XMVector3TransformCoord(Vec3(-1.f, 1.f, 1.f), matInv),
			::XMVector3TransformCoord(Vec3(1.f, 1.f, 1.f), matInv),
			::XMVector3TransformCoord(Vec3(1.f, -1.f, 1.f), matInv),
			::XMVector3TransformCoord(Vec3(-1.f, -1.f, 1.f), matInv)
		};

		Planes[PLANE_FRONT] = ::XMPlaneFromPoints(worldPos[0], worldPos[1], worldPos[2]); // CW
		Planes[PLANE_BACK] = ::XMPlaneFromPoints(worldPos[4], worldPos[7], worldPos[5]); // CCW
		Planes[PLANE_UP] = ::XMPlaneFromPoints(worldPos[4], worldPos[5], worldPos[1]); // CW
		Planes[PLANE_DOWN] = ::XMPlaneFromPoints(worldPos[7], worldPos[3], worldPos[6]); // CCW
		Planes[PLANE_LEFT] = ::XMPlaneFromPoints(worldPos[4], worldPos[0], worldPos[7]); // CW
		Planes[PLANE_RIGHT] = ::XMPlaneFromPoints(worldPos[5], worldPos[6], worldPos[1]); // CCW
	}

	bool ContainsSphere(const Vec3& pos, float radius)
	{
		for (const Vec4& plane : Planes)
		{
			// n = (a, b, c)
			Vec3 normal = Vec3(plane.x, plane.y, plane.z);

			// ax + by + cz + d > radius
			if (normal.Dot(pos) + plane.w > radius)
				return false;
		}

		return true;
	}
};

class CameraComponent : public Component<CameraComponent>
{
public:
	Matrix& GetViewMatrix()			{ return mView; }
	Matrix& GetProjectionMatrix()	{ return mProjection; }

	bool IntersectsOBB(const BoundingOrientedBox& obb) const { return mBoundingFrustum.Intersects(obb); }


	void SetNear(float value)	{ mNear = value; }
	void SetFar(float value)	{ mFar = value; }
	void SetFOV(float value)	{ mFov = value; }
	void SetScale(float value)	{ mScale = value; }
	void SetWidth(float value)	{ mWidth = value; }
	void SetHeight(float value) { mHeight = value; }



	void FinalUpdate(Matrix mat);

public:
	PROJECTION_TYPE mCameraType = PROJECTION_TYPE::PERSPECTIVE;

	float mNear = 1.f * 1.f;
	float mFar = 200.f * 100.f;

	float mShadowNear = 1.f;
	// 이동(카메라 추종) cascade 범위 — 이 너머는 맵 고정 4-cascade(정적 캐시)가 커버
	float mShadowFar = 10000.f;

	float mFov = 103.f/2.0f;// XM_PI / 4.f; (오버워치 fov로 맞춤)
	float mScale = 1.f;
	float mWidth = 2560.0f; //2560, 1440
	float mHeight = 1440.f;

	Matrix mView = {};
	Matrix mProjection = {};

	Frustum mFrustum;
	BoundingFrustum mBoundingFrustum;

	uint32 mCullingMask = 0;	//어떤 레이어를 랜더링할건지 비트로 계산
	CameraParams mCameraParams;
};

enum PlayMode
{
	MAIN_CAMERA,
	ONE_FPS,
	THREE_FPS,
	THREE_RPG,
};

class CameraTypeComponent : public Component<CameraTypeComponent>
{
public:
	CameraTypeComponent(PlayMode mode): mPlayMode(mode) {}
	CameraTypeComponent(EntityID targetID, PlayMode mode ): mPlayMode(mode), mTargetID(targetID) {}
public:
	EntityID mTargetID;
	PlayMode mPlayMode;
	float mCameraMoveSpeed = 100;

	SweepHit mCameraSweep{};

	float mCameraMaxLenth = 170;
	float mCameraMinLenth = 40;

	// cameraDistance에 따른 디더로 페이드 아웃
	float mCameraFadeStart = 150.f; // 알파 1 (불투명)
	float mCameraFadeEnd   = 70.f;	// 알파 0 (완전 컬링)
	float mCurrentFadeAlpha = 1.f;   // 프레임간 lerp 저장 (튐 방지)
	float mFadeLerpSpeed = 12.f;     // 페이드 보간 속도 (1/s)

	BoundingOrientedBox mCameraBox;
	float mCameraSphereRadius = 15.f;
	float mCameraMargin = 5.f;

	Vec3 mOffset = Vec3(80, 160, 0);
	Vec3 mLookAtOffset = Vec3(0, 0, -5);

	// F1 ONE_FPS: 머리 고정 오프셋(플레이어 yaw 공간). y=머리 높이, z=약간 앞.
	Vec3 mFpsHeadOffset = Vec3(0.f, 150.f, 15.f);

	// F4 MAIN_CAMERA(자유 카메라) 상태
	bool  mFreeCamInit  = false;
	Vec3  mFreeCamPos   = Vec3::Zero;
	float mFreeYaw      = 0.f;    // deg
	float mFreePitch    = 0.f;    // deg (+=85 클램프)
	float mFreeCamSpeed = 1300.f;  // 자유 비행 속도

	// 카메라 쉐이크
	float mShakeRemaining = 0.f;   // 남은 진동 시간
	float mShakeDuration  = 0.f;   // 초기 지속 시간 (decay 계산용)
	float mShakeMagnitude = 0.f;   // 최대 흔들림 각도 (degree)
	float mShakeFrequency = 20.f;  // 진동 주파수 (Hz)
	float mShakeTimeAcc   = 0.f;   // sin 누적 시간

	void TriggerShake(float magnitude, float duration, float frequency)
	{
		if (duration <= 0.f || magnitude <= 0.f || frequency <= 0.f)
			return;


		mShakeMagnitude = magnitude;
		mShakeDuration = duration;
		mShakeRemaining = duration;
		mShakeFrequency = frequency;
		mShakeTimeAcc = 0.f;
	}
};
