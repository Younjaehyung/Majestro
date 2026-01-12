#include "pch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "RenderManager.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "INetSendSink.h"
#include "Timer.h"
#include "Imgui.h"

Engine::Engine()
{
}

Engine::~Engine() = default;

void Engine::Initialize(const WindowInfo& info, shared_ptr<INetSendSink>& sendSink)
{
	mRenderManager = make_unique<RenderManager>();
	mResourceManager = make_unique<ResourceManager>();
	mSceneManager = make_unique<SceneManager>();
	mAudioManager = make_unique<AudioManager>();
	mInputManager = make_unique<InputManager>();

	
	mRenderManager->Initialize(info);
	mResourceManager->Initialize();

	mAudioManager->Initialize("..\\Resources\\Sound");

	mInputManager->Initialize(info.Hwnd);
	mTimer = make_unique<Timer>();
	mTimer->Start();

	mSceneManager->Initialize();
	mHwnd = info.Hwnd;

	mNetSendSink = sendSink;

#ifdef _IMGUI
	ImGuiManager::Get().Initialize(
		info.Hwnd,
		mRenderManager->GetDevice()->GetDevice().Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		mRenderManager->GetLegacyGraphicsDescriptorHeap(),
		mRenderManager->GetGraphicsCmdQueue()->GetCommandQueue().Get()
	);
#else
#endif
}

void Engine::Update()
{
	mRenderManager->StartRender();
	mTimer->Tick();
	mInputManager->Update();
	mSceneManager->Update(mTimer->GetTimeElapsed());
	mAudioManager->Update(mTimer->GetTimeElapsed());
}

void Engine::Render()
{
	
	mSceneManager->Render();

#ifdef _IMGUI
	ImGuiManager::Get().Render(mRenderManager->GetGraphicsCmdQueue()->GetGraphicsCmdList().Get(),
		mRenderManager->GetLegacyGraphicsDescriptorHeap());
#else
#endif
	mRenderManager->EndRender();


	ShowFps();
}


void Engine::ShowFps()
{
	uint32 fps = mTimer->GetFrameRate();

	WCHAR text[100] = L"";
	::wsprintf(text, L"FPS : %d", fps);

	::SetWindowText(mHwnd, text);
}