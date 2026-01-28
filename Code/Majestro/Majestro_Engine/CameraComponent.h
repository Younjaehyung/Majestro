#pragma once

#include "Component.h"

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

	float mNear = 1.f;
	float mFar = 10000.f;
	float mFov = XM_PI / 4.f;
	float mScale = 1.f;
	float mWidth = 1200.f;
	float mHeight = 800.f;

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
	CameraTypeComponent(ComponentTypeID targetID, PlayMode mode ): mPlayMode(mode), mTargetID(targetID) {}
public:
	ComponentTypeID mTargetID;
	PlayMode mPlayMode;
	float mCameraMoveSpeed = 100;
	float mCameraHight = 20;
	float mCameraLenth = 25;
};
