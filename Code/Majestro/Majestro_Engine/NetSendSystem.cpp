#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "InputManager.h"
#include "MovementComponent.h"
#include "TagComponent.h"

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
			//gSendBuffer.Push(seq);
			if (seq.Type != PKT_Type::KNONE)
			{
				gSendBuffer.Push(seq);
			}
			
			//netComp->mIsDirty = false;
		}
	}
}

void NetSendSystem::ConvertInput(SendRequest* seq)
{
	
	
	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponent<PlayerMovementComponent>();
	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);


	std::vector<Entity> choiceplayerEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
	ChoicePlayerComponent* characterChoice = mWorld->GetComponent<ChoicePlayerComponent>(choiceplayerEntities[0]);
	
	mInputPacket = C2S_InputPacket();
	mInputPacket.netEntityId = mWorld->GetComponent<NetEntityComponent>(playerEntity)->mNetEntityId;
	mInputPacket.MoveX = comp->mMovingDirection.x;
	mInputPacket.MoveY = comp->mJump;
	mInputPacket.MoveZ = comp->mMovingDirection.z;
	mInputPacket.Yaw = comp->mCameraRotationY;
	mInputPacket.Pitch = comp->mCameraRotationX;
	
	// buttons 마스킹

	/*if (comp->mAttack)			mInputPacket.Buttons |= InputButtons::SPACE;
	if (comp->mDash)			mInputPacket.Buttons |= INPUT_DASH;
	if (comp->mInteract)			mInputPacket.Buttons |= INPUT_INTERACT;*/
	if (comp->mJump)			mInputPacket.Buttons |= static_cast<uint8>(InputButtons::SPACE);

	if (!mHasSentGameStart && INPUT.GetKeyDown(eKeyCode::R))
	{
		cout << "start Game" << endl;
		const uint32 clientId = Network::GetInstance().mClientId;
		C2S_StartGamePacket startPacket;
		startPacket.clientId = clientId;
		startPacket.playerType = characterChoice ? characterChoice->mPlayerType : 0;
		startPacket.SessionId = clientId;
		startPacket.Sequence = 0;

		SendRequest startSeq;
		startSeq.Type = PKT_Type::C2S_GAME_START;
		startSeq.SIze = sizeof(C2S_StartGamePacket);
		startSeq.StoreAs(startPacket);
		gSendBuffer.Push(startSeq);
		mHasSentGameStart = true;
	}

	// Convert InputComponent data to SendRequest format
	seq->Type = PKT_Type::C2S_PKT_INPUT;
	seq->SIze = sizeof(C2S_InputPacket);
	
	seq->StoreAs(mInputPacket);
}