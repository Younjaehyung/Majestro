#include "pch.h"
#include "TransformComponent.h"
#include "Entity.h"
#include "MathUtils.h"

void TransformComponent::LookAt(const Vec3 dir)
{
	Vec3 front = dir;
	front.Normalize();

	Vec3 right = Vec3::Up.Cross(dir);
	if (right == Vec3::Zero)
		right = Vec3::Forward.Cross(dir);

	right.Normalize();

	Vec3 up = front.Cross(right);
	up.Normalize();

	Matrix matrix = XMMatrixIdentity();
	matrix.Right(right);
	matrix.Up(up);
	matrix.Backward(front);

	mLocalRotationE = DecomposeRotationMatrix(matrix);
}

bool TransformComponent::CloseEnough(const float& a, const float& b, const float& epsilon)
{
	return (epsilon > std::abs(a - b));
}

Vec3 TransformComponent::DecomposeRotationMatrix(const Matrix& rotation)
{
	Vec4 v[4];
	XMStoreFloat4(&v[0], rotation.Right());
	XMStoreFloat4(&v[1], rotation.Up());
	XMStoreFloat4(&v[2], rotation.Backward());
	XMStoreFloat4(&v[3], rotation.Translation());

	Vec3 ret;
	if (CloseEnough(v[0].z, -1.0f))
	{
		float x = 0;
		float y = XM_PI / 2;
		float z = x + atan2(v[1].x, v[2].x);
		ret = Vec3{ x, y, z };
	}
	else if (CloseEnough(v[0].z, 1.0f))
	{
		float x = 0;
		float y = -XM_PI / 2;
		float z = -x + atan2(-v[1].x, -v[2].x);
		ret = Vec3{ x, y, z };
	}
	else
	{
		float y1 = -asin(v[0].z);
		float y2 = XM_PI - y1;

		float x1 = atan2f(v[1].z / cos(y1), v[2].z / cos(y1));
		float x2 = atan2f(v[1].z / cos(y2), v[2].z / cos(y2));

		float z1 = atan2f(v[0].y / cos(y1), v[0].x / cos(y1));
		float z2 = atan2f(v[0].y / cos(y2), v[0].x / cos(y2));

		if ((std::abs(x1) + std::abs(y1) + std::abs(z1)) <= (std::abs(x2) + std::abs(y2) + std::abs(z2)))
		{
			ret = Vec3{ x1, y1, z1 };
		}
		else
		{
			ret = Vec3{ x2, y2, z2 };
		}
	}

	return ret;
}

void TransformComponent::UpdateRotationQuaternionFromEuler(Vec3 rotation)
{
	mLocalRotationR = MathUtils::EulerDegreesToRadians(rotation);
	mLocalRotationQ = MathUtils::QuatFromEulerDegrees(rotation);
}

void TransformComponent::UpdateRotationQuaternionFromEuler()
{
	mLocalRotationR = MathUtils::EulerDegreesToRadians(mLocalRotationE);
	mLocalRotationQ = MathUtils::QuatFromEulerDegrees(mLocalRotationE);
}

void TransformComponent::UpdateRotationEulerFromQuaternion()
{
	mLocalRotationR = mLocalRotationQ.ToEuler();
	mLocalRotationE.x = DirectX::XMConvertToDegrees(mLocalRotationR.x);
	mLocalRotationE.y = DirectX::XMConvertToDegrees(mLocalRotationR.y);
	mLocalRotationE.z = DirectX::XMConvertToDegrees(mLocalRotationR.z);
}

Matrix TransformComponent::FinalUpdate(Matrix parentMatrix)
{
		UpdateRotationQuaternionFromEuler();

		mLocalMatScale = Matrix::CreateScale(mLocalScale);
		mLocalMatRotation = Matrix::CreateFromQuaternion(mLocalRotationQ);
		mLocalMatTranslation = Matrix::CreateTranslation(mLocalPosition);

		/*Matrix matRotation = Matrix::CreateRotationX(mLocalRotationE.x);
		matRotation *= Matrix::CreateRotationY(mLocalRotationE.y);
		matRotation *= Matrix::CreateRotationZ(mLocalRotationE.z);*/
		

		mLocalMatrix = mLocalMatScale * mLocalMatRotation * mLocalMatTranslation;
		mWorldMatrix = mLocalMatrix * parentMatrix;
		mWorldPosition = mWorldMatrix.Translation();
		
		mIsDirty = true;
	
		return mWorldMatrix;
}
