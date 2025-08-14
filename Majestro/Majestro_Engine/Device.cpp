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



#ifdef _DEBUG
	// bindless check (Tier 2/3)
	D3D12_FEATURE_DATA_D3D12_OPTIONS opt{};
	if (SUCCEEDED(mDXDevice.Get()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opt, sizeof(opt))))
		cout << opt.ResourceBindingTier << "TIER" << '\n';



#endif

}
