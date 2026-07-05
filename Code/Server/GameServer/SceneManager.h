#pragma once
#include "PacketHelper.h"
class Scene;
class RoomManager;

enum {
	MAX_LAYER = 32
};

// TryChangeScene 결과. 응답 패킷 송신은 ScenePacketHandler 책임.
struct SceneChangeOutcome
{
	bool sendResponse = false;                       // false = 응답 불필요 (디버그 강제 전환 등)
	bool approved = false;
	SceneId resultScene = SceneId::Lobby;            // 승인 시 target, 거부 시 현재 씬
	RoomErrorCode roomError = RoomErrorCode::None;   // 로비 게임시작 자격 거부 사유 (None = 해당 없음)
	uint32 errorRoomId = 0;                          // roomError 통지용 roomId
	std::vector<uint64> alsoNotifySessions;          // 함께 승인 통지할 나머지 방원
};

class SceneManager
{
public:
	void Initialize();
	void Update(float deltaTime);
	void InitializeSession(uint64 sessionId);
	void RemoveSession(uint64 sessionId);
	void LoadScene(uint64 sessionId, wstring sceneName);

	// 게임플레이 입력을 세션이 속한 World 의 입력 큐로 전달 (분배는 PacketRouter 담당)
	bool EnqueueToWorld(const InputCommand& command);

	// 씬 전환 요청 검증·적용. 패킷 파싱/응답은 ScenePacketHandler 가 수행한다.
	SceneChangeOutcome TryChangeScene(uint64 sessionId, SceneId requestedScene);

	shared_ptr<Scene> GetScene(uint64 sessionId) const;
	shared_ptr<Scene> GetGameWorld(uint32 roomId) const;	// 룸별 게임 World 조회


	void SetRoomManager(RoomManager* roomManager) { mRoomManager = roomManager; }

private:

	bool IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const;
	

	// 방의 게임 World 가 더 이상 InGame 세션을 갖지 않으면 Release
	void CleanupRoomWorldIfEmpty(uint32 roomId);

	// 방을 유지한 채 게임 World 를 target 게임 씬으로 교체하고 방원 전원에게 전환 통지
	void SwapRoomWorldTo(uint32 roomId, SceneId target);

	SceneId GetOrCreateSceneState(uint64 sessionId);

	void SetLayerName(uint8 index, const wstring& name);



	const wstring& IndexToLayerName(uint8 index) { return _layerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);

private:
	void FactoryScene();	// 전체 씬을 생성하는 함수

	// 씬 분류는 Packet.h 의 IsLevelScene / IsRoomScene 공용 헬퍼 사용

	shared_ptr<Scene> CreateSceneById(SceneId id);	// 방 단위 씬(광장/레벨) 인스턴스 생성.

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