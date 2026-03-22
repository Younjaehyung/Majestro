#pragma once
#include "PacketHelper.h"
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


	bool EnqueueCommand(const InputCommand& command);
private:
	//std::vector<Scene*> mScene;

	void LoadScene(SceneId id);
	bool HandleSceneChange(const InputCommand& command);
	bool IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const;
	SceneId GetOrCreateSceneState(uint64 sessionId);


	void SetLayerName(uint8 index, const wstring& name);
	const wstring& IndexToLayerName(uint8 index) { return _layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);
public:
	shared_ptr<Scene> GetActiveScene() { return mActiveScene; }

private:
	void FactoryScene();	// 전체 씬을 생성하는 함수 (예: 로비 씬, 게임 씬 등)
	
	// Scene 전환
	void TransitionToScene();	// 씬 전환을 처리하는 함수 (예: 로비에서 게임으로, 게임에서 로비로 등)
private:

	//std::unordered_map<uint64, shared_ptr<Scene>> mScenesBySession;
	std::unordered_map<uint64, shared_ptr<Scene>> mLobbyScenesBySession;
	std::unordered_map<uint64, SceneId> mSceneBySession;

	std::array<shared_ptr<Scene>, (size_t)SceneId::End> mGameScenes; // 모든 씬을 저장하는 배열
	shared_ptr<Scene> mActiveScene;


	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> _layerNames;
	map<wstring, uint8> _layerIndex;

};