#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "Prefab.h"
#include "PlayerComponent.h"
#include <unordered_set>

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
	mPhase = SysPhase::Pre;
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;

	while (processed < kMaxMsgsPerTick && mWorld->DequeueCommand(mInputCommand)) {
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
				//LoginProcess(mInputCommand);
				//LoginProcess(mInputCommand, false);
				cout << "login process" << endl;
				break;
			}
			case PKT_Type::C2S_GAME_START:
			{

				//LoginProcess(mInputCommand);
				LoginProcess(mInputCommand, true);
				EnemySpawnProcess(mInputCommand);
				BulletPoolSpawnProcess(mInputCommand);

				break;
			}
			/*case PKT_Type::C2S_SCENE_CHANGE:
			{
				HandleSceneChange(mInputCommand);
				break;
			}*/
		}
		++processed;
		
	}
}

void NetRecvSystem::RecvInput(uint32 sessionId, const C2S_InputPacket& inputFrame)
{
	if (!mWorld->HasComponentPool<InputComponent>() || !mWorld->HasComponentPool<NetEntityComponent>())
		return;

	auto view = mWorld->GetEntitiesWithComponent<InputComponent>();
	for (auto entity : view)
	{
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (inputComp == nullptr || netComp == nullptr)
			continue;

		if (netComp->mSessionId == sessionId)
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

void NetRecvSystem::LoginProcess(InputCommand& inputCommand, bool broadcastToWorld) 
{
	///uint32 ssessionId = 0;//mInputCommand.SessionId;
	uint8 playertype = 1;

	if (inputCommand.Type == PKT_Type::C2S_GAME_START)
	{
		const C2S_StartGamePacket* startPacket = inputCommand.ViewAs<C2S_StartGamePacket>();
		if (startPacket)
		{
			playertype = startPacket->playerType;
			cout <<"charactor: " << (int)startPacket->playerType << endl;
		}
	}

	if (playertype > 2)
	{
		playertype = 1;
	}


	Entity e = PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, inputCommand);
	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
	MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(e);
	if (playerComp)
	{
		playerComp->mPlayerType = playertype;
	}

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
	if (broadcastToWorld){
		for (uint32 sessionId : CollectPlayerSessions())
		{
			if (sessionId == inputCommand.SessionId)
				continue;

			SendRequest request{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			spawnPkt.isLocalPlayer = 0;
			request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
			gSendQueue.Push(request);
		}
	}

	// 로그인 한 클라이언트에게 이미 접속해있는 다른 플레이어들에 대한 Spawn 패킷 전송
	auto entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	for (auto entity : entities) {
		

		netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		playerComp = mWorld->GetComponent<MainPlayerComponent>(entity);

		if (nullptr == netComp)
			continue;
		
		if (inputCommand.SessionId == netComp->mSessionId || netComp->mSessionId == 0)
			continue;


		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(netComp->mSessionId, netComp->mNetEntityId, PrefabType::PLAYER);
		spawnPkt.isPlayerType = playerComp ? playerComp->mPlayerType : 1;

		SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
		request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
		gSendQueue.Push(request);
	}

}

void NetRecvSystem::EnemySpawnProcess(InputCommand& inputCommand)
{
	
	if (mEnemySpawnOnce) {
		mNetEntityIds.reserve(200);
		for (int i = 0;i < 10; ++i) {
			Entity e = PrefabFactory::Spawn(mWorld, PrefabType::ENEMY, inputCommand);
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);
			mNetEntityIds.push_back(netComp->mNetEntityId);

		}
		mEnemySpawnOnce = false;
	}

	for (uint64 id :mNetEntityIds) {
		S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(inputCommand.SessionId, id, PrefabType::ENEMY);

		// 로그인 한 클라이언트에게 자신의 Spawn 패킷 전송
		for (uint32 sessionId : CollectPlayerSessions())
		{
			//SendRequest request{ inputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			SendRequest request{ sessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
			spawnPkt.isLocalPlayer = 0;
			request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
			gSendQueue.Push(request);
		}
	}
}

void NetRecvSystem::BulletPoolSpawnProcess(InputCommand& inputCommand)
{
	if (!mBulletSpawnOnce)
		return;

	constexpr int kBulletPoolSize = 64;
	for (int i = 0; i < kBulletPoolSize; ++i)
	{
		PrefabFactory::Spawn(mWorld, PrefabType::BULLET, inputCommand);
	}

	mBulletSpawnOnce = false;
}

std::vector<uint32> NetRecvSystem::CollectPlayerSessions() const
{
	std::unordered_set<uint32> sessionSet;
	if (false == mWorld->HasComponentPool<NetEntityComponent>())
		return {};

	auto entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	sessionSet.reserve(entities.size());
	for (auto entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (!netComp || netComp->mSessionId == 0)
			continue;

		sessionSet.insert(netComp->mSessionId);
	}

	return std::vector<uint32>(sessionSet.begin(), sessionSet.end());
}