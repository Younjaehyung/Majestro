#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "InputManager.h"
#include "MovementComponent.h"
#include <bitset>

NetSendSystem::NetSendSystem(World* world, EventManager* event) : System::System(world, event)
{

}

void NetSendSystem::Update(double deltaTime)
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();

	for(auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);

		if (netComp == nullptr) continue;
		//if (netComp->mIsDirty)
		{
			SendRequest seq;
			ConvertInput(&seq);
			gSendBuffer.Push(seq);
			
			//netComp->mIsDirty = false;
		}
	}
}

void NetSendSystem::ConvertInput(SendRequest* seq)
{
	
	
	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponent<PlayerMovementComponent>();
	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
	
	mInputPacket = C2S_InputPacket();
	mInputPacket.netEntityId = mWorld->GetComponent<NetEntityComponent>(playerEntity)->mNetEntityId;
	mInputPacket.MoveX = comp->mMovingDirection.x;
	mInputPacket.MoveY = comp->mJump;
	mInputPacket.MoveZ = comp->mMovingDirection.z;
	mInputPacket.Yaw = comp->mCameraRotationY;
	mInputPacket.Pitch = comp->mCameraRotationX;
	
	// buttons 비트마스크로 변환

	/*
	if (comp->mDash)			mInputPacket.Buttons |= INPUT_DASH;
	if (comp->mInteract)			mInputPacket.Buttons |= INPUT_INTERACT;*/
	if (comp->mJump)			mInputPacket.Buttons |= (1 << static_cast<uint8>(InputButtons::SPACE));
	if (comp->mAttack1)			mInputPacket.Buttons |= (1 << static_cast<uint8>(InputButtons::ATTACK));



	// Convert InputComponent data to SendRequest format
	seq->Type = PKT_Type::C2S_PKT_INPUT;
	seq->SIze = sizeof(C2S_InputPacket);
	
	seq->StoreAs(mInputPacket);
} 
