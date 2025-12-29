#include "pch.h"
#include "Engine.h"

#include "SceneManager.h"
#include "RenderManager.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "Imgui.h"

Engine::Engine()
{
}

Engine::~Engine() = default;

void Engine::Initialize(const WindowInfo& info)
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


	ImGuiManager::Get().Initialize(
		info.Hwnd,
		mRenderManager->GetDevice()->GetDevice().Get(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		mRenderManager->GetLegacyGraphicsDescriptorHeap(),
		mRenderManager->GetGraphicsCmdQueue()->GetCommandQueue().Get()
	);
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

	ImGuiManager::Get().BeginFrame();
	ImGuiManager::Get().Render();
	ImGuiManager::Get().EndFrame(mRenderManager->GetGraphicsCmdQueue()->GetGraphicsCmdList().Get(),
		mRenderManager->GetLegacyGraphicsDescriptorHeap());

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