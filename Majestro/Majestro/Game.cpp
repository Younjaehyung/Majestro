#include "pch.h"
#include "Game.h"
#include "Engine.h"

void Game::Initialize(const WindowInfo& info)
{
	gEngine->Initialize(info);
}

void Game::Update()
{

	gEngine->Update();
}

void Game::Render()
{
	gEngine->Render();
}
