#include "pch.h"
#include "Engine.h"
#include "SceneManager.h"
#include "RenderManager.h"
#include "InputManager.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "Timer.h"
#include "Imgui.h"
#include "Chat.h"
#include "EngineLog.h"

namespace
{
	struct FrameCpuProfile
	{
		EngineLog::CpuProfileSample frame;
		EngineLog::CpuProfileSample start;
		EngineLog::CpuProfileSample update;
		EngineLog::CpuProfileSample render;
		EngineLog::CpuProfileSample end;
		EngineLog::CpuTimePoint lastFrame{};
		uint32 measuredFrames = 0;
		uint32 frameIntervals = 0;
		bool hasLastFrame = false;

		void ResetWindow()
		{
			frame.Reset();
			start.Reset();
			update.Reset();
			render.Reset();
			end.Reset();
			measuredFrames = 0;
			frameIntervals = 0;
		}
	};

	FrameCpuProfile sFrameCpuProfile;
}

Engine::Engine()
{
}

Engine::~Engine() = default;

void Engine::Initialize(const WindowInfo& info)
{
	// 엔진 재초기화 시 이전 실행의 성능 누적값을 남기지 않는다.
	sFrameCpuProfile = {};

	mRenderManager = make_unique<RenderManager>();
	mResourceManager = make_unique<ResourceManager>();
	mSceneManager = make_unique<SceneManager>();
	mAudioManager = make_unique<AudioManager>();
	mInputManager = make_unique<InputManager>();

	mRenderManager->Initialize(info);
	mResourceManager->Initialize();

	mAudioManager->Initialize("..\\Resources\\Sound");

	mInputManager->Initialize(info.Hwnd);
	mInputManager->AddKeyInputSuppressor([] { return Chat::Get().ShouldBlockGameInput(); });

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
#endif
}

void Engine::Update()
{
	// 리소스 전환은 StartRender 이전에 처리해 업로드와 명령 할당기 충돌을 피한다.
	if (EngineLog::Enabled(EngineLog::Domain::FramePerformance))
	{
		const EngineLog::CpuTimePoint now = EngineLog::CpuNow();
		if (sFrameCpuProfile.hasLastFrame)
		{
			sFrameCpuProfile.frame.Add(
				EngineLog::CpuElapsedMilliseconds(
					sFrameCpuProfile.lastFrame,
					now));
			sFrameCpuProfile.frameIntervals++;
		}
		sFrameCpuProfile.lastFrame = now;
		sFrameCpuProfile.hasLastFrame = true;
	}

	mSceneManager->ProcessCommands();

	{
		EngineLog::ScopedCpuProfile profile(
			EngineLog::Domain::FramePerformance,
			sFrameCpuProfile.start);
		mRenderManager->StartRender();
	}

	mTimer->Tick();
	mInputManager->Update();

	{
		EngineLog::ScopedCpuProfile profile(
			EngineLog::Domain::FramePerformance,
			sFrameCpuProfile.update);
		mSceneManager->Update(mTimer->GetTimeElapsed());
	}

	mAudioManager->Update(mTimer->GetTimeElapsed());
}

void Engine::Render()
{
#ifdef _IMGUI
	// 렌더 시스템이 현재 프레임의 ImGui 데이터를 기록할 수 있도록 먼저 시작한다.
	ImGuiManager::Get().BeginFrame();
#endif

	{
		EngineLog::ScopedCpuProfile profile(
			EngineLog::Domain::FramePerformance,
			sFrameCpuProfile.render);
		mSceneManager->Render();
	}

#ifdef _IMGUI
	ImGuiManager::Get().EndFrame(
		mRenderManager->GetGraphicsCmdQueue()->GetGraphicsCmdList().Get(),
		mRenderManager->GetLegacyGraphicsDescriptorHeap());
#endif

	{
		EngineLog::ScopedCpuProfile profile(
			EngineLog::Domain::FramePerformance,
			sFrameCpuProfile.end);
		mRenderManager->EndRender();
	}

	if (EngineLog::Enabled(EngineLog::Domain::FramePerformance) &&
		++sFrameCpuProfile.measuredFrames >= 60)
	{
		const double frameMs =
			sFrameCpuProfile.frame.Average(sFrameCpuProfile.frameIntervals);
		wchar_t buffer[256]{};
		swprintf_s(
			buffer,
			L"[CPU] Frame %.2f ms (%.0f fps) | Start/wait %.2f | Update/logic %.2f | Render/record %.2f | End/present %.2f\n",
			frameMs,
			frameMs > 0.0 ? 1000.0 / frameMs : 0.0,
			sFrameCpuProfile.start.Average(sFrameCpuProfile.measuredFrames),
			sFrameCpuProfile.update.Average(sFrameCpuProfile.measuredFrames),
			sFrameCpuProfile.render.Average(sFrameCpuProfile.measuredFrames),
			sFrameCpuProfile.end.Average(sFrameCpuProfile.measuredFrames));
		EngineLog::OutputPerformance(
			EngineLog::Domain::FramePerformance,
			buffer);
		sFrameCpuProfile.ResetWindow();
	}

	ShowFps();
}

void Engine::ShowFps()
{
	uint32 fps = mTimer->GetFrameRate();

	WCHAR text[100] = L"";
	::wsprintf(text, L"FPS : %d", fps);

	::SetWindowText(mHwnd, text);
}
