#include "pch.h"
#include "NetSendSystem.h"
#include "World.h"
#include "NetEntityComponent.h"
#include "TransformComponent.h"

NetSendSystem::NetSendSystem(World* world) : System(world)
{
}

void NetSendSystem::Update(float dt)
{
	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
	
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		//if (netComp->mIsDirty)
		{
			ConvertMove(netComp, &mSendReq);
		//	netComp->mIsDirty = false;
			gSendQueue.Push(mSendReq);
		}
	}
}

void NetSendSystem::ConvertMove(NetEntityComponent* netComp, SendRequest* seq)
{
	
	TransformComponent* transComp = mWorld->GetComponent<TransformComponent>(netComp->mOwnerEntity);
	seq->SessionId = 0;
	seq->Type = S2C_PKT_MOVE;
	seq->Size = sizeof(S2C_MovePacket);

	S2C_MovePacket movePkt;
	movePkt.netEntityId = netComp->mNetEntityId;
	movePkt.x = transComp->mWorldPosition.x;
	movePkt.y = transComp->mWorldPosition.y;
	movePkt.z = transComp->mWorldPosition.z;
	movePkt.yaw = transComp->mLocalRotation.y;
	movePkt.pitch = transComp->mLocalRotation.x;
	std::cout << movePkt.netEntityId << " Send Move Packet Position: (" << movePkt.x << ", " << movePkt.y << ", " << movePkt.z << "), Yaw: " << movePkt.yaw << ", Pitch: " << movePkt.pitch << std::endl;
	seq->StoreAs<S2C_MovePacket>(movePkt);
}







