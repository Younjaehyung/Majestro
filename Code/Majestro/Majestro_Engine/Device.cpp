#include "pch.h"
#include "Device.h"

#ifndef ENABLE_D3D12_DEBUG_LAYER
#define ENABLE_D3D12_DEBUG_LAYER 0
#endif

void Device::Initialize()
{

#if defined(_DEBUG) && ENABLE_D3D12_DEBUG_LAYER
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDXDebug))) && mDXDebug)
		mDXDebug->EnableDebugLayer();
#endif

	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG) && ENABLE_D3D12_DEBUG_LAYER
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mDXGIFactory));

	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		ComPtr<IDXGIAdapter1> candidate;
		if (FAILED(mDXGIFactory->EnumAdapters1(adapterIndex, &candidate)))
			break;

		DXGI_ADAPTER_DESC1 desc{};
		candidate->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDXDevice))))
		{
			candidate.As(&mAdapter);
			break;
		}
	}

	if (mDXDevice == nullptr)
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
