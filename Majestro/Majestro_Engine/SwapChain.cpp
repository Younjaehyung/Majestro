#include "pch.h"
#include "SwapChain.h"

void SwapChain::Initialize(const WindowInfo& info, ComPtr<ID3D12Device> device, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	CreateSwapChain(info, dxgi, cmdQueue);
}

void SwapChain::Present()
{
	mSwapChain->Present(0, 0);
}

void SwapChain::SwapIndex()
{
	mBackBufferIndex = (mBackBufferIndex + 1) % SWAP_CHAIN_BUFFER_COUNT;
}

void SwapChain::CreateSwapChain(const WindowInfo& info, ComPtr<IDXGIFactory> dxgi, ComPtr<ID3D12CommandQueue> cmdQueue)
{
	// 이전에 만든 정보 날린다
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC SWAP_CHAIN_DESC{};									//스왑체인 설정 시작.
	SWAP_CHAIN_DESC.BufferDesc.Width = static_cast<uint32>(info.Width);		// 버퍼의 해상도 너비
	SWAP_CHAIN_DESC.BufferDesc.Height = static_cast<uint32>(info.Height);	// 버퍼의 해상도 높이
	SWAP_CHAIN_DESC.BufferDesc.RefreshRate.Numerator = 60;					// 화면 갱신 비율
	SWAP_CHAIN_DESC.BufferDesc.RefreshRate.Denominator = 1;					// 화면 갱신 비율
	SWAP_CHAIN_DESC.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// 버퍼의 디스플레이 형식(RGBA32비트형식)
	SWAP_CHAIN_DESC.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	SWAP_CHAIN_DESC.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SWAP_CHAIN_DESC.SampleDesc.Count = 4; // 멀티 샘플링
	SWAP_CHAIN_DESC.SampleDesc.Quality = 0;
	SWAP_CHAIN_DESC.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 후면 버퍼에 렌더링할 것 
	SWAP_CHAIN_DESC.BufferCount = SWAP_CHAIN_BUFFER_COUNT; // 전면+후면 버퍼
	SWAP_CHAIN_DESC.OutputWindow = info.Hwnd;
	SWAP_CHAIN_DESC.Windowed = info.ScreenState;
	SWAP_CHAIN_DESC.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 전면 후면 버퍼 교체 시 이전 프레임 정보 버림
	SWAP_CHAIN_DESC.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	

	dxgi->CreateSwapChain(cmdQueue.Get(), &SWAP_CHAIN_DESC, &mSwapChain);	//스왑체인 만들기

	dxgi->MakeWindowAssociation(info.Hwnd, DXGI_MWA_NO_ALT_ENTER);	// alt + enter 전체화면 금지

}
