#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "InputManager.h"
#include "Network.h"

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
