#pragma once
class Scene;

enum class LoadingVisualType
{
	Startup,
	GameStart
};

enum {
	MAX_LAYER = 32
};

class SceneManager
{
public:
	void Initialize();
	void Update(float deltaTime);
	void Render();

	void LoadScene(wstring sceneName);

	void QueueLoadScene(const wstring& sceneName);
	void QueueGameStartAfterLoad();
	void StorePendingPlayerType(uint8 playerType);
	void QueueLoadingScene(const wstring& loadingMessage, LoadingVisualType visualType = LoadingVisualType::GameStart);
	void SetLoadingMessage(const wstring& loadingMessage);
	const wstring& GetLoadingMessage() const { return mLoadingMessage; }
	bool IsLoading() const { return mIsLoading; }
	LoadingVisualType GetLoadingVisualType() const { return mLoadingVisualType; }

	bool HasPendingSceneChange() const { return mHasPendingSceneChange; }

private:
	std::vector<Scene*> mScene;


	void SetLayerName(uint8 index, const wstring& name);
	const wstring& IndexToLayerName(uint8 index) { return _layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);
public:
	shared_ptr<Scene> GetActiveScene() { return mActiveScene; }

private:
//	shared_ptr<Scene> LoadTestScene();

private:
	shared_ptr<Scene> mActiveScene;
	bool mHasPendingSceneChange = false;
	bool mHasPendingLoadingScene = false;
	wstring mPendingSceneName;
	wstring mActiveSceneName;
	bool mPendingGameStart = false;
	bool mHasPendingPlayerType = false;
	uint8 mPendingPlayerType = 0;
	bool mIsLoading = false;
	wstring mLoadingMessage;
	LoadingVisualType mLoadingVisualType = LoadingVisualType::Startup;

	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> _layerNames;
	map<wstring, uint8> _layerIndex;

};