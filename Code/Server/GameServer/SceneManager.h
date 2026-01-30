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
	void InitializeSession(uint64 sessionId);
	void RemoveSession(uint64 sessionId);
	void LoadScene(uint64 sessionId, wstring sceneName);
	shared_ptr<Scene> GetScene(uint64 sessionId) const;
private:
	std::vector<Scene*> mScene;

	void LoadScene(wstring sceneName);


	void SetLayerName(uint8 index, const wstring& name);
	const wstring& IndexToLayerName(uint8 index) { return _layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);
public:
	shared_ptr<Scene> GetActiveScene() { return mActiveScene; }

private:
//	shared_ptr<Scene> LoadTestScene();

private:
	shared_ptr<Scene> mActiveScene;
	std::unordered_map<uint64, shared_ptr<Scene>> mScenesBySession;

	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> _layerNames;
	map<wstring, uint8> _layerIndex;

};