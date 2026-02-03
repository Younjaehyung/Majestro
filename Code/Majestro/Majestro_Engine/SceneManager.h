#pragma once
class Scene;

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
	wstring mPendingSceneName;
	bool mPendingGameStart = false;
	bool mHasPendingPlayerType = false;
	uint8 mPendingPlayerType = 0;

	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> _layerNames;
	map<wstring, uint8> _layerIndex;

};