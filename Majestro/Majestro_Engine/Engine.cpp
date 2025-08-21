#include "pch.h"
#include "Engine.h"

#include "SceneManager.h"
#include "RenderManager.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "Timer.h"

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

	mSceneManager->Initialize();
	mHwnd = info.Hwnd;
}

void Engine::Update()
{
	
	mTimer->Tick();
	mInputManager->Update();
	mSceneManager->Update(mTimer->GetTimeElapsed());
	mAudioManager->Update(mTimer->GetTimeElapsed());
}

void Engine::Render()
{
	mRenderManager->StartRender();
	mSceneManager->Render();

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