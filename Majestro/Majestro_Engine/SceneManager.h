#pragma once
#include <array> 

class SceneManager
{
public:
	void Initialize();
	void Update(float deltaTime);
	void Render();
private:
	array<Scene*> mScene;
};

