#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;
	while (processed < kMaxMsgsPerTick && gRecvQueue.Pop(mInputCommand)) {
		cout << "update" << endl;
		switch (mInputCommand.Type)
		{
			case PKT_Type::C2S_PKT_INPUT:
			{
				const C2S_InputPacket* inputFrame = mInputCommand.ViewAs<C2S_InputPacket>();
				if (inputFrame)
				{
					
					RecvInput(mInputCommand.SessionId, *inputFrame);
				}
				break;
			}
			case PKT_Type::C2S_PKT_LOGIN:
			{
				LoginProcess(mInputCommand);
				cout << "login process" << endl;
				//EnemySpawnProcess(mInputCommand);
				break;
			}
			case PKT_Type::C2S_GAME_START:
			{
				S2C_StartGamePacket startPacket(mInputCommand.SessionId);
				SendRequest request{ mInputCommand.SessionId, PKT_Type::S2C_GAME_START, sizeof(S2C_StartGamePacket) };
				request.StoreAs<S2C_StartGamePacket>(startPacket);
				gSendQueue.Push(request);

				LoginProcess(mInputCommand);
				EnemySpawnProcess(mInputCommand);
				break;
			}
			case PKT_Type::C2S_SCENE_CHANGE:
			{
				HandleSceneChange(mInputCommand);
				break;
			}
		}
		++processed;
		
	}
}

void NetRecvSystem::RecvInput(uint32 sessionId, const C2S_InputPacket& inputFrame)
{
	auto view = mWorld->GetEntitiesWithComponent<InputComponent>();
	for (auto entity : view)
	{
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId == sessionId)
		{
			//std::cout << "[NetRecvSystem] C2S_PKT_INPUT received from SessionID: " << mInputCommand.SessionId << std::endl;

			// 중복/역순 입력 방지
			/*if (inputFrame.Seq <= inputComp->lastSeq)
				return;*/
			inputComp->MoveX = inputFrame.MoveX;
			inputComp->MoveY = inputFrame.MoveY;
			inputComp->MoveZ = inputFrame.MoveZ;
			inputComp->Buttons = inputFrame.Buttons;
			inputComp->Yaw = inputFrame.Yaw;
			inputComp->Pitch = inputFrame.Pitch;
			inputComp->lastSeq = inputFrame.Seq;

			break;
		}
	}
}

void NetRecvSystem::LoginProcess(InputCommand& inputCommand)
{
	uint32 ssessionId = 0;//mInputCommand.SessionId;
	uint8 playertype = 1;
	mSceneBySession[inputCommand.SessionId] = SceneId::Lobby;

	Entity e = PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, inputCommand);
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);

	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(inputCommand.SessionId, netComp->mNetEntityId, PrefabType::PLAYER);
	spawnPkt.isPlayerType = playertype;

	// 로그인 한 클라이언트에게 자신의 Spawn 패킷 전송
	{
		SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		spawnPkt.isLocalPlayer = 1;
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}

	// 다른 클라이언트들에게 로그인 한 클라이언트의 Spawn 패킷 전송
	{
		SendRequest request{ ssessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		spawnPkt.isLocalPlayer = 0;
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}
	std::cout << "[YSW]Spawn Packet Sent to SessionID: " << ssessionId << netComp->mNetEntityId << std::endl;


	// 로그인 한 클라이언트에게 이미 접속해있는 다른 플레이어들에 대한 Spawn 패킷 전송
	for (auto& N : mWorld->GetNetIdMap()->GetNetIdMap()) {
		
		netComp = mWorld->GetComponent<NetEntityComponent>(N.second);
		

		if(nullptr == mWorld->GetComponent<MainPlayerComponent>(N.second)|| nullptr== netComp)
			continue;
		
		if (inputCommand.SessionId == netComp->mSessionId || netComp->mSessionId == 0)
			continue;


		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(netComp->mSessionId, netComp->mNetEntityId, PrefabType::PLAYER);
		spawnPkt.isPlayerType = playertype;

		SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}

}

void NetRecvSystem::HandleSceneChange(InputCommand& inputCommand)
{
	const C2S_SceneChangePacket* requestPacket = inputCommand.ViewAs<C2S_SceneChangePacket>();
	if (!requestPacket)
		return;

	SceneId currentScene = GetOrCreateScene(inputCommand.SessionId);
	SceneId requestedScene = requestPacket->targetScene;
	const bool isApproved = IsSceneChangeAllowed(currentScene, requestedScene);
	if (isApproved)
	{
		mSceneBySession[inputCommand.SessionId] = requestedScene;
		currentScene = requestedScene;
	}

	S2C_SceneChangeResultPacket responsePacket(currentScene, isApproved);
	SendRequest response{ inputCommand.SessionId, PKT_Type::S2C_SCENE_CHANGE_RESULT, sizeof(S2C_SceneChangeResultPacket) };
	response.StoreAs<S2C_SceneChangeResultPacket>(responsePacket);
	gSendQueue.Push(response);
}

bool NetRecvSystem::IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const
{
	if (currentScene == requestedScene)
		return false;

	switch (currentScene)
	{
	case SceneId::Lobby:
		return requestedScene == SceneId::Game;
	case SceneId::Game:
		return requestedScene == SceneId::Lobby;
	default:
		return false;
	}
}

SceneId NetRecvSystem::GetOrCreateScene(uint32 sessionId)
{
	auto findIt = mSceneBySession.find(sessionId);
	if (findIt != mSceneBySession.end())
		return findIt->second;

	mSceneBySession[sessionId] = SceneId::Lobby;
	return SceneId::Lobby;
}


void NetRecvSystem::EnemySpawnProcess(InputCommand& inputCommand)
{
	uint32 ssessionId = 0;//mInputCommand.SessionId;


	if (mEnemySpawnOnce) {
		mNetEntityIds.reserve(200);
		for (int i = 0;i < 5; ++i) {
			Entity e = PrefabFactory::Spawn(mWorld, PrefabType::ENEMY, inputCommand);
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
			mNetEntityIds.push_back(netComp->mNetEntityId);

		}
		mEnemySpawnOnce = false;
	}

	for (uint64 id :mNetEntityIds) {
		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(inputCommand.SessionId, id, PrefabType::ENEMY);

		// 로그인 한 클라이언트에게 자신의 Spawn 패킷 전송
		{
			SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			spawnPkt.isLocalPlayer = 0;
			request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
			gSendQueue.Push(request);
		}
	}
}

