#include "pch.h"
#include "RenderManager.h"
#include "SceneManager.h"

void RenderManager::Initialize(const WindowInfo& info)
{
	mWindow = info;

	mViewport = { 0, 0, static_cast<FLOAT>(info.Width), static_cast<FLOAT>(info.Height), 0.0f, 1.0f };	//뷰포트창 세팅
	mScissorRect = CD3DX12_RECT(0, 0, info.Width, info.Height);	//사각형 생성

	mDevice->Initialize();
	mGraphicsCommandQueue->Initialize(mDevice->GetDevice(), mSwapChain);
	//_computeCmdQueue->Init(_device->GetDevice());
	mSwapChain->Initialize(info, mDevice->GetDevice(), mDevice->GetDXGI(), mGraphicsCommandQueue->GetCommandQueue());
	
	//_rootSignature->Initialize();
	//_graphicsDescHeap->Initialize(256);
	//_computeDescHeap->Initialize();
}

void RenderManager::Update()
{
}

void RenderManager::StartRender()
{
	mGraphicsCommandQueue->RenderBegin();
}


void RenderManager::EndRender()
{
	mGraphicsCommandQueue->RenderEnd();
}

void RenderManager::ResizeWindow(int32 width, int32 height)
{
	mWindow.Width = width;
	mWindow.Height = height;

	//윈도우 창 사이즈 조절
	RECT rect = { 0, 0, width, height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	SetWindowPos(mWindow.Hwnd, 0, 100, 100, width, height, 0);

}