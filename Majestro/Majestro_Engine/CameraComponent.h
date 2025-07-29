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
	array<Vec4, PLANE_END> _planes;


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

		_planes[PLANE_FRONT] = ::XMPlaneFromPoints(worldPos[0], worldPos[1], worldPos[2]); // CW
		_planes[PLANE_BACK] = ::XMPlaneFromPoints(worldPos[4], worldPos[7], worldPos[5]); // CCW
		_planes[PLANE_UP] = ::XMPlaneFromPoints(worldPos[4], worldPos[5], worldPos[1]); // CW
		_planes[PLANE_DOWN] = ::XMPlaneFromPoints(worldPos[7], worldPos[3], worldPos[6]); // CCW
		_planes[PLANE_LEFT] = ::XMPlaneFromPoints(worldPos[4], worldPos[0], worldPos[7]); // CW
		_planes[PLANE_RIGHT] = ::XMPlaneFromPoints(worldPos[5], worldPos[6], worldPos[1]); // CCW
	}

	bool ContainsSphere(const Vec3& pos, float radius)
	{
		for (const Vec4& plane : _planes)
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
	Matrix& GetViewMatrix() { return _matView; }
	Matrix& GetProjectionMatrix() { return _matProjection; }

	void SetNear(float value) { _near = value; }
	void SetFar(float value) { _far = value; }
	void SetFOV(float value) { _fov = value; }
	void SetScale(float value) { _scale = value; }
	void SetWidth(float value) { _width = value; }
	void SetHeight(float value) { _height = value; }



	void FinalUpdate(Matrix mat);
	PROJECTION_TYPE _type = PROJECTION_TYPE::PERSPECTIVE;

	float _near = 1.f;
	float _far = 10000.f;
	float _fov = XM_PI / 4.f;
	float _scale = 1.f;
	float _width = 0.f;
	float _height = 0.f;

	Matrix _matView = {};
	Matrix _matProjection = {};

	Frustum _frustum;


	uint32 _cullingMask = 0;	//어떤 레이어를 랜더링할건지 비트로 계산

};
