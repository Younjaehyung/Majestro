#pragma once

class SceneManager;
class RenderManager;
class ResourceManager;
class AudioManager;
class InputManager;
class Timer;

class INetSendSink;

class Engine
{
public:
	Engine();
	~Engine();

	//static Engine* GetInstance() {
	//	static Engine instance;
	//	return &instance;
	//}
	void Initialize(const WindowInfo& info, shared_ptr<INetSendSink>& sendSink);
	void Update();
	void Render();
	void ShowFps();
public:
	
public:
	RenderManager&		GetRenderManager()		{ return *mRenderManager; }
	SceneManager&		GetSceneManager()		{ return *mSceneManager; }
	ResourceManager&	GetResourceManager()	{ return *mResourceManager; }
	AudioManager&		GetAudioManager()		{ return *mAudioManager; }
	InputManager&		GetInputManager()		{ return *mInputManager; }
	Timer&				GetTimer()				{ return *mTimer; }
	
	shared_ptr<INetSendSink>& GetNetSendSink()	{ return mNetSendSink; }
private:

	unique_ptr<RenderManager>	mRenderManager;
	unique_ptr<SceneManager>	mSceneManager;
	unique_ptr<ResourceManager> mResourceManager;
	unique_ptr<AudioManager>	mAudioManager;
	unique_ptr<InputManager>	mInputManager;
	unique_ptr<Timer>			mTimer;

	shared_ptr<INetSendSink>	mNetSendSink;

	HWND mHwnd;
};
