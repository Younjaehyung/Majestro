#include "pch.h"
#include "NetSendSystem.h"
#include "World.h"
#include "NetEntityComponent.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "MovementComponent.h"
#include "BulletComponent.h"
#include "HealthComponent.h"
#include <unordered_set>

NetSendSystem::NetSendSystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

void NetSendSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	ConvertMove(mNetComp, &mSendReq, dt);	//move
	ConvertState();
	SendCollision();
	SendHealthIfChanged();
	
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

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();
	for (auto& entity : entities)
	{
		netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr) continue;
		if (mWorld->HasComponent<BulletComponent>(entity))
			continue;
		{

			TransformComponent* transComp = mWorld->GetComponent<TransformComponent>(netComp->mOwnerEntity);

			EnemyMovementComponent* enemyMovementComponent = mWorld->GetComponent<EnemyMovementComponent>(netComp->mOwnerEntity);
			/*seq->SessionId = 0;
			seq->Type = S2C_PKT_MOVE;
			seq->Size = sizeof(S2C_MovePacket);*/

			S2C_MovePacket movePkt;

			movePkt.netEntityId = netComp->mNetEntityId;
			movePkt.x = transComp->mWorldPosition.x;
			movePkt.y = transComp->mWorldPosition.y;
			movePkt.z = transComp->mWorldPosition.z;

			movePkt.vx = transComp->mMovingVector.x;
			movePkt.vy = transComp->mMovingVector.y;
			movePkt.vz = transComp->mMovingVector.z;
			
			movePkt.rx = transComp->mLocalRotationQ.x;
			movePkt.ry = transComp->mLocalRotationQ.y;
			movePkt.rz = transComp->mLocalRotationQ.z;
			movePkt.rw = transComp->mLocalRotationQ.w;


			movePkt.yaw = transComp->mLocalRotationE.y;
			movePkt.pitch = transComp->mLocalRotationE.x;
			seq->StoreAs<S2C_MovePacket>(movePkt);

			
			
			
			//	netComp->mIsDirty = false;
			//gSendQueue.Push(mSendReq);

			for (uint32 sessionId : recipients)
			{
				seq->SessionId = sessionId;
				seq->Type = S2C_PKT_MOVE;
				seq->Size = sizeof(S2C_MovePacket);
				seq->StoreAs<S2C_MovePacket>(movePkt);
				//	netComp->mIsDirty = false;
				gSendQueue.Push(mSendReq);
			}
		}
	}


}

void NetSendSystem::ConvertState()
{
	if (false == mWorld->HasComponentPool<MainPlayerComponent>())return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	for (auto& entity : entities)
	{
		//if (netComp->mIsDirty)
		{

			/*mSendReq.SessionId = 0;
			mSendReq.Type = S2C_PKT_STATE;
			mSendReq.Size = sizeof(S2C_StatePacket);*/
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			MainPlayerComponent* playerComp = mWorld->GetComponent<MainPlayerComponent>(netComp->mOwnerEntity);
			S2C_StatePacket statePkt;
			statePkt.netEntityId = netComp->mNetEntityId;
			statePkt.stateId = playerComp->GetState();
			statePkt.lowerStateId = playerComp->GetLowerState();

			for (uint32 sessionId : recipients)
			{
				mSendReq.SessionId = sessionId;
				mSendReq.Type = S2C_PKT_STATE;
				mSendReq.Size = sizeof(S2C_StatePacket);
				mSendReq.StoreAs<S2C_StatePacket>(statePkt);
				//	netComp->mIsDirty = false;
				gSendQueue.Push(mSendReq);
			}
		}

	}
}

void NetSendSystem::SendCollision()
{
	if (false == mWorld->HasComponentPool<BoxColliderComponent>())return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<BoxColliderComponent, NetEntityComponent>();
	for (auto& entity : entities)
	{
		//if (netComp->mIsDirty)
		{

			/*mSendReq.SessionId = 0;
			mSendReq.Type = S2C_PKT_STATE;
			mSendReq.Size = sizeof(S2C_CollisionPacket);*/
			S2C_CollisionPacket collisionPkt;
			NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
			collisionPkt.netEntityId = netComp->mNetEntityId;
			collisionPkt.bIsColliding = mWorld->GetComponent<BoxColliderComponent>(entity)->bIsColliding;
			//mSendReq.StoreAs<S2C_CollisionPacket>(collisionPkt);
			////	netComp->mIsDirty = false;
			//gSendQueue.Push(mSendReq);

			for (uint32 sessionId : recipients)
			{
				mSendReq.SessionId = sessionId;
				mSendReq.Type = S2C_PKT_STATE;
				mSendReq.Size = sizeof(S2C_CollisionPacket);
				mSendReq.StoreAs<S2C_CollisionPacket>(collisionPkt);
				//	netComp->mIsDirty = false;
				gSendQueue.Push(mSendReq);
			}
		}
	}
}


std::vector<uint32> NetSendSystem::CollectPlayerSessions() const
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



void NetSendSystem::SendHealthIfChanged()
{
	if (false == mWorld->HasComponentPool<HealthComponent>())
		return;

	auto recipients = CollectPlayerSessions();
	if (recipients.empty())
		return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponents<NetEntityComponent, HealthComponent>();
	for (auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr)
			continue;

		HealthComponent* healthComp = mWorld->GetComponent<HealthComponent>(entity);
		if (healthComp == nullptr)
			healthComp = mWorld->GetComponent<HealthComponent>(netComp->mOwnerEntity);
		if (healthComp == nullptr)
			continue;

		const uint64 netEntityId = netComp->mNetEntityId;
		const std::pair<int32, int32> currentHealth{ healthComp->mCurrentHp, healthComp->mMaxHp };
		auto it = mLastSentHealthByNetEntity.find(netEntityId);
		if (it != mLastSentHealthByNetEntity.end() && it->second == currentHealth)
			continue;

		mLastSentHealthByNetEntity[netEntityId] = currentHealth;

		S2C_HealthPacket healthPkt;
		healthPkt.netEntityId = netEntityId;
		healthPkt.currentHp = currentHealth.first;
		healthPkt.maxHp = currentHealth.second;

		for (uint32 sessionId : recipients)
		{
			mSendReq.SessionId = sessionId;
			mSendReq.Type = S2C_PKT_HEALTH;
			mSendReq.Size = sizeof(S2C_HealthPacket);
			mSendReq.StoreAs<S2C_HealthPacket>(healthPkt);
			gSendQueue.Push(mSendReq);
		}
	}
}

