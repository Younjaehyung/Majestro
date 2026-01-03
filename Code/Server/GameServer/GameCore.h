#pragma once
#include "SceneManager.h"

class GameCore
{
public:
	void Initialize();
	void Start();
	void Update(float deltaTime);

private:
	void LoadGameData();
	void UpdateGameLogic(float deltaTime);

private:
	std::unique_ptr<SceneManager> mSceneManager;
};

