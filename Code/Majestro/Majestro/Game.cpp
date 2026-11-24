#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "InputManager.h"
#include "Imgui.h"
#include "Network.h"
#include "Scene.h"
#include "SceneManager.h"
#include "QaBot.h"
#include "McpBridge.h"
#include "McpInput.h"

#ifdef _IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
#endif


void Game::Initialize(const WindowInfo& info)
{

	mInitializeEnd = true;

	gEngine->Initialize(info);

	// -qa: 사람 대신 QA 봇이 입력을 공급한다. 엔진은 QA 를 모르고, 배선은 실행 파일이 한다.
	// -mcpinput (또는 -qa 와 -mcp 를 같이): MCP press/move_to 가 진행 중일 때 그 입력이 QaBot 입력을 덮어쓴다.
	// 입력 소스를 등록하면 하드웨어 입력이 꺼지므로, 둘 다 아니면 등록하지 않는다.
	const bool qa = QaBot::Requested();
	const bool mcpInput = McpBridge::Requested() && (qa || McpBridge::InputRequested());
	if (mcpInput)
		McpInput::Get().Enable();
	if (qa || mcpInput)
	{
		gEngine->GetInputManager().SetInputSource([qa, mcpInput](InputFrame& frame) {
			if (qa)
				QaBot::Get().ProduceInput(frame);
			if (mcpInput)
				McpInput::Get().Apply(frame);
		});
	}

	Network::GetInstance().Initialize();
	
}

void Game::Update()
{
	// -mcp: 외부 MCP 명령을 프레임 시작에 게임 스레드에서 처리한다.
	if (McpBridge::Requested())
		McpBridge::Get().Poll();

	gEngine->Update();

}

void Game::Input(UINT message, WPARAM wParam, LPARAM lParam)
{
	gEngine->GetInputManager().OnMouseEvent(message, wParam, lParam);
}


void Game::ActiveGame(bool active)
{
	if(mInitializeEnd)gEngine->GetInputManager().OnActivateApp(active);
}

void Game::Render()
{
	gEngine->Render();
}

bool Game::IsGameSceneActive() const
{
	if (!mInitializeEnd)
		return false;

	auto activeScene = gEngine->GetSceneManager().GetActiveScene();
	if (!activeScene)
		return false;

	auto world = activeScene->GetWorld();
	if (!world)
		return false;

	return true;
}

int Game::ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;
#else
#endif
	return false;
}
