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

// ── CPU 프로파일 (Update 로직 vs Render 기록) — 임시 측정용 ──
static double Engine_QpcMs(long long ticks)
{
	static LARGE_INTEGER freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
	return static_cast<double>(ticks) * 1000.0 / static_cast<double>(freq.QuadPart);
}
static double sCpuUpdateMs   = 0.0;   // SceneManager::Update  (ECS 게임로직)
static double sCpuRenderMs   = 0.0;   // SceneManager::Render  (커맨드 리스트 기록)
static double sCpuStartMs    = 0.0;   // StartRender           (WaitForFrame/BackBuffer = GPU 완료 대기)
static double sCpuEndMs      = 0.0;   // EndRender             (Execute + Present)
static double sCpuFrameMs    = 0.0;   // 전체 프레임 간격
static long long sCpuLastFrameQpc = 0;
static int    sCpuProfFrames = 0;

void Engine::Update()
{
	// 씬 전환은 StartRender() 이전에 처리
	// GPU 커맨드 리스트(BeginCommandList)가 열리기 전에 씬 초기화가 완료되어야
	// Effekseer Effect::Create() GPU 업로드와 커맨드 얼로케이터 충돌을 방지
	// 전체 프레임 간격 (직전 Update 진입 시각 대비)
	LARGE_INTEGER fNow; QueryPerformanceCounter(&fNow);
	if (sCpuLastFrameQpc != 0)
		sCpuFrameMs += Engine_QpcMs(fNow.QuadPart - sCpuLastFrameQpc);
	sCpuLastFrameQpc = fNow.QuadPart;

	mSceneManager->ProcessCommands();

	LARGE_INTEGER sA; QueryPerformanceCounter(&sA);
	mRenderManager->StartRender();                      // WaitForBackBuffer/WaitForFrame (GPU 완료 대기 포함)
	LARGE_INTEGER sB; QueryPerformanceCounter(&sB);
	sCpuStartMs += Engine_QpcMs(sB.QuadPart - sA.QuadPart);

	mTimer->Tick();
	mInputManager->Update();

	LARGE_INTEGER uA; QueryPerformanceCounter(&uA);
	mSceneManager->Update(mTimer->GetTimeElapsed());   // ECS 로직 (애니/물리/AI)
	LARGE_INTEGER uB; QueryPerformanceCounter(&uB);
	sCpuUpdateMs += Engine_QpcMs(uB.QuadPart - uA.QuadPart);

	mAudioManager->Update(mTimer->GetTimeElapsed());
}

void Engine::Render()
{
#ifdef _IMGUI
	// NewFrame()을 씬 렌더 이전에 호출해야 IMGUIRenderSystem::Update()의 위젯이 프레임에 포함됨
	ImGuiManager::Get().BeginFrame();
#endif

	LARGE_INTEGER rA; QueryPerformanceCounter(&rA);
	mSceneManager->Render();                            // 커맨드 리스트 기록
	LARGE_INTEGER rB; QueryPerformanceCounter(&rB);
	sCpuRenderMs += Engine_QpcMs(rB.QuadPart - rA.QuadPart);

#ifdef _IMGUI
	ImGuiManager::Get().EndFrame(mRenderManager->GetGraphicsCmdQueue()->GetGraphicsCmdList().Get(),
		mRenderManager->GetLegacyGraphicsDescriptorHeap());
#else
#endif

	LARGE_INTEGER eA; QueryPerformanceCounter(&eA);
	mRenderManager->EndRender();                        // Execute + Present
	LARGE_INTEGER eB; QueryPerformanceCounter(&eB);
	sCpuEndMs += Engine_QpcMs(eB.QuadPart - eA.QuadPart);

	if (++sCpuProfFrames >= 60)
	{
		const double f = sCpuFrameMs / sCpuProfFrames;
		wchar_t _buf[256];
		swprintf_s(_buf,
			L"[CPU] Frame %.2f ms (%.0f fps) | Start/wait %.2f | Update/logic %.2f | Render/record %.2f | End/present %.2f\n",
			f, (f > 0.0 ? 1000.0 / f : 0.0),
			sCpuStartMs / sCpuProfFrames, sCpuUpdateMs / sCpuProfFrames,
			sCpuRenderMs / sCpuProfFrames, sCpuEndMs / sCpuProfFrames);
		OutputDebugStringW(_buf);
		sCpuUpdateMs = sCpuRenderMs = sCpuStartMs = sCpuEndMs = sCpuFrameMs = 0.0;
		sCpuProfFrames = 0;
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