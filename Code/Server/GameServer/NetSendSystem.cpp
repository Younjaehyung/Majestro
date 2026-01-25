#include "pch.h"
#include "NetSendSystem.h"
#include "World.h"
#include "NetEntityComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"

NetSendSystem::NetSendSystem(World* world) : System(world)
{
}

void NetSendSystem::Update(float dt)
{

	ConvertMove(mNetComp, &mSendReq, dt);	//move
	ConvertState();

}

void NetSendSystem::ConvertMove(NetEntityComponent* netComp, SendRequest* seq, float dt)
{
	
	mMoveSendAccumulator += dt;
	if (mMoveSendAccumulator < mMoveSendInterval)
	{
		return;
	}

	if (mMoveSendAccumulator > (mMoveSendInterval * mMaxMoveBurst))
	{
		mMoveSendAccumulator = mMoveSendInterval * mMaxMoveBurst;
	}
	mMoveSendAccumulator -= mMoveSendInterval;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
	for (auto& entity : entities)
	{
		netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		//if (netComp->mIsDirty)
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
			
			
			
			//	netComp->mIsDirty = false;
			gSendQueue.Push(mSendReq);
		}
	}


}

void NetSendSystem::ConvertState()
{
	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	for (auto& entity : entities)
	{
		//if (netComp->mIsDirty)
		{

			mSendReq.SessionId = 0;
			mSendReq.Type = S2C_PKT_STATE;
			mSendReq.Size = sizeof(S2C_PKT_STATE);
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(netComp->mOwnerEntity);
			S2C_StatePacket statePkt;
			statePkt.netEntityId = netComp->mNetEntityId;
			statePkt.stateId = playerComp->GetState();
			mSendReq.StoreAs<S2C_StatePacket>(statePkt);
			//	netComp->mIsDirty = false;
			gSendQueue.Push(mSendReq);
		}
	}
}







