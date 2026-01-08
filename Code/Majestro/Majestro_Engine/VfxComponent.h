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

	//effect_ = LoadEffect(u"..\\Resources\\Effect\\fire.efk");

	float	mTotalTime = 0.f;
	bool	mIsPlaying = false;
	::Effekseer::Vector3D mPosition = ::Effekseer::Vector3D(0.0f, 0.0f, 0.0f);

	//manager_->AddLocation(efkHandle, ::Effekseer::Vector3D(-1.2f, 3.0f, 0.0f));
};

