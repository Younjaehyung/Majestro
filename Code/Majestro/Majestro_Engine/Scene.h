#pragma once
#include "World.h"
class SystemManager;

class Scene
{
public:
	virtual ~Scene() = default;

	virtual void Initialize();

	virtual void Update(float deltaTime);
	virtual void Render();

	const shared_ptr<World>& GetWorld() { return mWorld; }

protected:

	shared_ptr<World>				mWorld = make_shared<World>();

};


class LobbyScene : public Scene
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
};

class GameScene : public Scene
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
};