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
	virtual void Shudown();

	void LoadJsonLevel(const wstring& path);
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

class LoadingScene : public Scene
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;

private:
	Entity mLoadingText = NULL_ENTITY;
	Entity mLoadingImage = NULL_ENTITY;
};

class GameScene : public Scene
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render() override;
};