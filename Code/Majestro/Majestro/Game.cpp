#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "InputManager.h"
#include "Imgui.h"
#include "Network.h"
#include "Scene.h"
#include "SceneManager.h"

#ifdef _IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
#endif


void Game::Initialize(const WindowInfo& info)
{

	mInitializeEnd = true;

	gEngine->Initialize(info);

	Network::GetInstance().Initialize();
	Network::GetInstance().Awake();
}

void Game::Update()
{
	ReceiveNetworkData();
	gEngine->Update();
	SendNetworkData();
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

	return world->GetSceneId() == SceneId::Game;
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

void Game::ReceiveNetworkData()
{
	
}

void Game::SendNetworkData()
{
	/*SendRequest sendCommand;

	sendCommand.Type = C2S_PKT_INPUT;
	if(gEngine->GetInputManager().GetKey(eKeyCode::A))
	sendCommand.input.x = -1;
	if(gEngine->GetInputManager().GetKey(eKeyCode::D))
	sendCommand.input.x = 1;

	if(gEngine->GetInputManager().GetKey(eKeyCode::W))
	sendCommand.input.y = 1;

	if(gEngine->GetInputManager().GetKey(eKeyCode::S))
	sendCommand.input.y = -1;

	gSendBuffer.Push(sendCommand);*/
}
