#pragma once

class SceneManager;
class RenderManager;
class ResourceManager;
class InputManager;
class Timer;

class Engine
{
public:
	Engine();
	~Engine();

	//static Engine* GetInstance() {
	//	static Engine instance;
	//	return &instance;
	//}
	void Initialize(const WindowInfo& info);
	void Update();
	void Render();
	void ShowFps();
public:
	RenderManager&		GetRenderManager()		{ return *mRenderManager; }
	SceneManager&		GetSceneManager()		{ return *mSceneManager; }
	ResourceManager&	GetResourceManager()	{ return *mResourceManager; }
	InputManager&		GetInputManager()		{ return *mInputManager; }
	Timer&				GetTimer()				{ return *mTimer; }

private:

	unique_ptr<RenderManager>	mRenderManager;
	unique_ptr<SceneManager>	mSceneManager;
	unique_ptr<ResourceManager> mResourceManager;
	unique_ptr<InputManager>	mInputManager;
	unique_ptr<Timer>			mTimer;


	HWND mHwnd;
};
