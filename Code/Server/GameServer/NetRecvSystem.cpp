#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"
#include "Prefab.h"

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;
	while (processed < kMaxMsgsPerTick && gRecvQueue.Pop(mInputCommand)) {
		std::cout << "nae" << endl;
		switch (mInputCommand.Type)
		{
			case PKT_Type::C2S_PKT_INPUT:
			{
				const InputFrame* inputFrame = mInputCommand.ViewAs<InputFrame>();
				if (inputFrame)
				{
					RecvInput(mInputCommand.SessionId, *inputFrame);
				}
				break;
			}
			case PKT_Type::C2S_PKT_LOGIN:
			{
				LoginProcess(0);
				break;
			}
		}
		++processed;
		
	}
}

void NetRecvSystem::RecvInput(uint32 sessionId, const InputFrame& inputFrame)
{
	auto view = mWorld->GetEntitiesWithComponent<InputComponent>();
	for (auto entity : view)
	{
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId == sessionId)
		{
			// 중복/역순 입력 방지
			if (inputFrame.Seq <= inputComp->lastSeq)
				return;
			inputComp->MoveX = inputFrame.MoveX;
			inputComp->MoveY = inputFrame.MoveY;
			inputComp->Buttons = inputFrame.Buttons;
			inputComp->Yaw = inputFrame.Yaw;
			inputComp->Pitch = inputFrame.Pitch;
			inputComp->lastSeq = inputFrame.Seq;
			break;
		}
	}
}

void NetRecvSystem::LoginProcess(uint32 sessionId)
{
	uint32 ssessionId = 0;//mInputCommand.SessionId;


	Entity e = PrefabFactory::Spawn(mWorld, PrefabType::PLAYER, mInputCommand);

	NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);

	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(mInputCommand.SessionId, netComp->mNetEntityId, PrefabType::PLAYER);

	SendRequest request{ ssessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
	request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
	gSendQueue.Push(request);
	std::cout << "[YSW]Spawn Packet Sent to SessionID: " << ssessionId << netComp->mNetEntityId << std::endl;



	//for (auto& N : mWorld->GetNetIdMap()->GetNetIdMap()) {
	//	netComp = mWorld->GetComponent<NetEntityComponent>(N.second);
	//	S2C_SpawnPacekt spawnPkt = S2C_SpawnPacekt(mInputCommand.SessionId, netComp->mNetEntityId, PrefabType::PLAYER);

	//	SendRequest request{ mInputCommand.SessionId, PKT_Type::S2C_PKT_SPAWN, sizeof(S2C_SpawnPacekt) };
	//	request.StoreAs<S2C_SpawnPacekt>(spawnPkt);
	//	gSendQueue.Push(request);
	//}

}

