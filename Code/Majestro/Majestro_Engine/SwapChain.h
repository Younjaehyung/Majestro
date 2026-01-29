#pragma once
class SwapChain
{
public:
	void Initialize(const WindowInfo& info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue);
	void Present();
	void SwapIndex();
	void UpdateBackBufferIndex();


	ComPtr<IDXGISwapChain3> GetSwapChain() { return mSwapChain; }
	uint32 GetBackBufferIndex() { return mBackBufferIndex; }
	uint32 GetCurrentBackBufferIndex() {return (mBackBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;}
private:
	void CreateSwapChain(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue);
	

private:

	ComPtr<IDXGISwapChain3>	mSwapChain;
	uint32					mBackBufferIndex = 0;
};


