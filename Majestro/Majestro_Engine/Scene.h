#pragma once

class GameObject;
class World;
class SystemManager;

class Scene
{
public:
	void Initialize();

	void Update(float deltaTime);

	void ClearRTV();
	void RenderShadow();
	void RenderDeferred();


	void RenderLights();
	void RenderFinal();

	void RenderForward();


	const shared_ptr<class World>& GetWorld() { return mWorld; }

private:
	void PushLightData();

private:

	vector<shared_ptr<class Camera>>	_cameras;
	vector<shared_ptr<class Light>>		_lights;

	shared_ptr<class World>				mWorld;

};