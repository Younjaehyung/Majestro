#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "InputManager.h"
#include "Imgui.h"
#include "Network.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void Game::Initialize(const WindowInfo& info)
{
	gEngine->Initialize(info);

	//Network::GetInstance().Initialize();
	//Network::GetInstance().ConnectToServer();
}

void Game::Update()
{
	//Network::GetInstance().Update();
	gEngine->Update();
	
}

void Game::Input(UINT message) 
{
	gEngine->GetInputManager().OnMouseMove(message);
}

void Game::Render()
{
	gEngine->Render();
}

int Game::ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return false;
}
