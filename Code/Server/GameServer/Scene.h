#pragma once


#include "World.h"
class SystemManager;

class Scene
{
public:
	void Initialize();

	void Update(float deltaTime);
	void LoadJsonLevel(const wstring& path);

	const shared_ptr<World>& GetWorld() { return mWorld; }


private:

	shared_ptr<World>				mWorld = make_shared<World>();

};