#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "InputManager.h"

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
		if (netComp->mIsDirty)
		{
			SendRequest seq;
			ConvertInput(&seq);
			gSendBuffer.Push(seq);
			
			netComp->mIsDirty = false;
		}
	}
}

void NetSendSystem::ConvertInput(SendRequest* seq)
{
	mInputPacket = C2S_InputPacket();
	mInputPacket.netEntityId = 0; // Set appropriate net entity ID
	mInputPacket.MoveX = 0.0f;
	mInputPacket.MoveY = 0.0f;

	
	if(INPUT.GetKeyDown(eKeyCode::A))
	{
		mInputPacket.MoveX -= 1.0f;
	}
	else if(INPUT.GetKeyDown(eKeyCode::D))
	{
		mInputPacket.MoveX += 1.0f;
	}
	if(INPUT.GetKeyDown(eKeyCode::W))
	{
		mInputPacket.MoveY += 1.0f;
	}
	else if(INPUT.GetKeyDown(eKeyCode::S))
	{
		mInputPacket.MoveY -= 1.0f;
	}





	// Convert InputComponent data to SendRequest format
	seq->Type = PKT_Type::C2S_PKT_INPUT;
	seq->SIze = sizeof(C2S_InputPacket);
	seq->StoreAs(mInputPacket);
}