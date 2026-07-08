#pragma once
class Device
{
public:

	void Initialize();

	ComPtr<IDXGIFactory> GetDXGI() { return mDXGIFactory; }
	ComPtr<ID3D12Device> GetDevice() { return mDXDevice; }
	ComPtr<IDXGIAdapter3> GetAdapter() { return mAdapter; }

private:
	ComPtr<ID3D12Device>	mDXDevice;
	ComPtr<IDXGIAdapter3>	mAdapter;
	ComPtr<IDXGIFactory6>	mDXGIFactory;
	ComPtr<ID3D12Debug>		mDXDebug;
};

