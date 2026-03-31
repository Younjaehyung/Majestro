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

	float mNear = 1.f * 100.f;
	float mFar = 800.f * 100.f;

	float mShadowNear = 1.f;
	float mShadowFar = 8000.f;

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

	float mCameraMaxLenth = 250;
	float mCameraMinLenth = 6;

	BoundingOrientedBox mCameraBox;
	float mCameraSphereRadius = 25.f;
	float mCameraMargin = 5.f;

	Vec3 mOffset = Vec3(80, 160, 0);
	Vec3 mLookAtOffset = Vec3(0, 0, -5);

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
