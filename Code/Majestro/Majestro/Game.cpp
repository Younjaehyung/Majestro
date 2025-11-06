#include "pch.h"
#include "Game.h"
#include "Engine.h"
#include "InputManager.h"

void Game::Initialize(const WindowInfo& info)
{
	gEngine->Initialize(info);
}

void Game::Update()
{
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
