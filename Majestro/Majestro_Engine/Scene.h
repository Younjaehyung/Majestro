#pragma once


#include "World.h"
class SystemManager;

class Scene
{
public:
	void Initialize();

	void Update(float deltaTime);
	void Render();

	const shared_ptr<World>& GetWorld() { return mWorld; }



private:

	vector<shared_ptr<class Camera>>	_cameras;
	vector<shared_ptr<class Light>>		_lights;

	shared_ptr<World>				mWorld = make_shared<World>();

};