#pragma once

class GameObject;
class World;
class SystemManager;

class Scene
{
public:
	void Initialize();

	void Update(float deltaTime);
	void Render();

	const shared_ptr<class World>& GetWorld() { return mWorld; }



private:

	vector<shared_ptr<class Camera>>	_cameras;
	vector<shared_ptr<class Light>>		_lights;

	shared_ptr<class World>				mWorld;

};