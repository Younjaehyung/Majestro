#include "pch.h"
#include "CameraComponent.h"

void CameraComponent::FinalUpdate(Matrix mat)
{
	mView = mat;

	if (mCameraType == PROJECTION_TYPE::PERSPECTIVE)
		mProjection = ::XMMatrixPerspectiveFovLH(mFov, mWidth / mHeight, mNear, mFar);
	else
		mProjection = ::XMMatrixOrthographicLH(mWidth * mScale, mHeight * mScale, mNear, mFar);


	//mFrustum.FinalUpdate(mView, mProjection);

	

	BoundingFrustum localFrustum;
	BoundingFrustum::CreateFromMatrix(localFrustum, mProjection);
	Matrix viewInv = mView.Invert();
	localFrustum.Transform(mBoundingFrustum, viewInv);
}
