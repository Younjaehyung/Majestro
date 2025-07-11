#include "pch.h"
#include "Device.h"

void Device::Initialize()
{

#ifdef _DEBUG
	D3D12GetDebugInterface(IID_PPV_ARGS(&mDXDebug));
	mDXDebug->EnableDebugLayer();
#endif
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&mDXGIFactory));
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDXDevice));
}
