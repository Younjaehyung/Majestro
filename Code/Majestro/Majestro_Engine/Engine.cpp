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
	// 씬 전환은 StartRender() 이전에 처리
	// GPU 커맨드 리스트(BeginCommandList)가 열리기 전에 씬 초기화가 완료되어야
	// Effekseer Effect::Create() GPU 업로드와 커맨드 얼로케이터 충돌을 방지
	mSceneManager->ProcessCommands();
	mRenderManager->StartRender();
	mTimer->Tick();
	mInputManager->Update();
	mSceneManager->Update(mTimer->GetTimeElapsed());
	mAudioManager->Update(mTimer->GetTimeElapsed());
}

void Engine::Render()
{
#ifdef _IMGUI
	// NewFrame()을 씬 렌더 이전에 호출해야 IMGUIRenderSystem::Update()의 위젯이 프레임에 포함됨
	ImGuiManager::Get().BeginFrame();
#endif

	mSceneManager->Render();

#ifdef _IMGUI
	ImGuiManager::Get().EndFrame(mRenderManager->GetGraphicsCmdQueue()->GetGraphicsCmdList().Get(),
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