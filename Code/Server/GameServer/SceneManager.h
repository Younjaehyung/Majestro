#pragma once
#include "PacketHelper.h"
class Scene;
class RoomManager;

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
	bool EnqueueCommand(const InputCommand& command);


	shared_ptr<Scene> GetScene(uint64 sessionId) const;
	shared_ptr<Scene> GetGameWorld(uint32 roomId) const;	// 룸별 게임 World 조회


	void SetRoomManager(RoomManager* roomManager) { mRoomManager = roomManager; }

private:

	bool HandleSceneChange(const InputCommand& command);
	bool IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const;
	

	// 방의 게임 World 가 더 이상 InGame 세션을 갖지 않으면 Release
	void CleanupRoomWorldIfEmpty(uint32 roomId);

	SceneId GetOrCreateSceneState(uint64 sessionId);

	void SetLayerName(uint8 index, const wstring& name);



	const wstring& IndexToLayerName(uint8 index) { return _layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);

private:
	void FactoryScene();	// 전체 씬을 생성하는 함수
	
	
	void TransitionToScene();	// Scene 전환

private:
	std::unordered_map<uint64, shared_ptr<Scene>> mLobbyScenesBySession;
	std::unordered_map<uint64, SceneId> mSceneBySession;

	std::array<shared_ptr<Scene>, (size_t)SceneId::End> mGameScenes; // 결과 씬 등 사전 생성 씬 보관 배열

	// roomId 별 게임 World 인스턴스.
	std::unordered_map<uint32 /*roomId*/, shared_ptr<Scene>> mGameWorldsByRoom;
	RoomManager* mRoomManager = nullptr;

	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> _layerNames;
	map<wstring, uint8> _layerIndex;

	
};