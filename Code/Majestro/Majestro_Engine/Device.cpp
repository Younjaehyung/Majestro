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


	//// 원하는 샘플 수 (보통 4)
	//UINT sampleCount = 4;

	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaInfo = {};
	//msaaInfo.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;                // 예: DXGI_FORMAT_R8G8B8A8_UNORM
	//msaaInfo.SampleCount = sampleCount;
	//msaaInfo.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	//msaaInfo.NumQualityLevels = 0;

	//HRESULT hr = mDXDevice.Get()->CheckFeatureSupport(
	//	D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
	//	&msaaInfo,
	//	sizeof(msaaInfo));

	//if (FAILED(hr))
	//{
	//	// 여기서 hr 로그 찍고 원인 파악해야 함.
	//	// 실패하면 msaaInfo.NumQualityLevels는 신뢰 불가.
	//}

	//bool msaaSupported = (msaaInfo.NumQualityLevels > 0);
	//UINT msaaQuality = msaaSupported ? (msaaInfo.NumQualityLevels - 1) : 0;


#ifdef _DEBUG
	// bindless check (Tier 2/3)
	D3D12_FEATURE_DATA_D3D12_OPTIONS opt{};
	if (SUCCEEDED(mDXDevice.Get()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opt, sizeof(opt))))
		cout << opt.ResourceBindingTier << "TIER" << '\n';



#endif

}
