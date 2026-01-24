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
		}
	}
}

void NetSendSystem::ConvertMove(NetEntityComponent* netComp, SendRequest* seq)
{
	
	TransformComponent* transComp = mWorld->GetComponent<TransformComponent>(netComp->mOwnerEntity);
	seq->SessionId = netComp->mSessionId;
	seq->Type = S2C_PKT_MOVE;
	seq->Size = sizeof(S2C_MovePacket);

	S2C_MovePacket movePkt{};
	movePkt.netEntityId = netComp->mNetEntityId;
	movePkt.x = transComp->mWorldPosition.x;
	movePkt.y = transComp->mWorldPosition.y;
	movePkt.z = transComp->mWorldPosition.z;

	//std::cout << "[NetSendSystem] S2C_PKT_MOVE sent to SessionID: " << seq->SessionId << " Position: (" << movePkt.x << ", " << movePkt.y << ", " << movePkt.z << ")" << std::endl;

	seq->StoreAs<S2C_MovePacket>(movePkt);
}







