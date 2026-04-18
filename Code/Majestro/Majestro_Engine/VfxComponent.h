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

	shared_ptr<Vfx> mVfx = nullptr;
	Effekseer::Handle efkHandle = -1;


	float	mTotalTime = 0.f;
	bool	mIsPlaying = false;
	bool	mIsPaused  = false;
	bool	mIsLoop    = false;
	Vec3	mScale     = Vec3(1.f, 1.f, 1.f);
	::Effekseer::Vector3D mPosition = ::Effekseer::Vector3D(0.0f, 0.0f, 0.0f);

};

