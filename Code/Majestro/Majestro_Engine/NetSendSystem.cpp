#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "EnginePch.h"
#include "Entity.h"
#include "World.h"
#include "Network.h"
#include "NetEntityComponent.h"
#include "InputManager.h"
#include "MovementComponent.h"
#include "TagComponent.h"

#include "TagComponent.h"
#include "SceneManager.h"

#include <bitset>


NetSendSystem::NetSendSystem(World* world) : System::System(world)
{
	mPhase = SysPhase::Pre;
}

void NetSendSystem::Update(float deltaTime)
{
	UpdateCachedPlayerType();
	TrySendGameStart();

	SendRequest seq;
	ConvertInput(&seq);
	if (seq.Type != PKT_Type::KNONE)
	{
		gSendBuffer.Push(seq);
	}

	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	std::vector<Entity> entities = mWorld->GetEntitiesWithComponent<NetEntityComponent>();

	for(auto& entity : entities)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);

		if (netComp == nullptr) continue;
		//if (netComp->mIsDirty)
		{
			
			//netComp->mIsDirty = false;
		}
	}
}

void NetSendSystem::ConvertInput(SendRequest* seq)
{
	if (INPUT.GetKeyDown(eKeyCode::G))
	{
		cout << "\ngame\n" << endl;
		SendSceneChange(SceneId::Game);
	}

	if (INPUT.GetKeyDown(eKeyCode::L))
	{
		cout << "\nloby\n" << endl;
		SendSceneChange(SceneId::Lobby);
	}


	if (false == mWorld->HasComponentPool<NetEntityComponent>())return;

	//cout << "input" << endl;
	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponent<PlayerMovementComponent>();
	if (playerEntities.empty())
	{
		seq->Type = PKT_Type::KNONE;
		return;
	}

	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
	
	NetEntityComponent* netEnt = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (!netEnt)
	{
		seq->Type = PKT_Type::KNONE; // [수정] 엔티티 할당 전엔 입력 전송 보류
		return;
	}

	mInputPacket = C2S_InputPacket();
	mInputPacket.netEntityId = mWorld->GetComponent<NetEntityComponent>(playerEntity)->mNetEntityId;
	mInputPacket.MoveX = comp->mMovingDirection.x;
	mInputPacket.MoveY = comp->mJump;
	mInputPacket.MoveZ = comp->mMovingDirection.z;
	mInputPacket.Yaw = comp->mCameraRotationY;
	mInputPacket.Pitch = comp->mCameraRotationX;
	
	// buttons 비트마스크로 변환

	/*
	
	if (comp->mInteract)			mInputPacket.Buttons |= INPUT_INTERACT;*/

	if (comp->mDash)			mInputPacket.Buttons |= (1 << static_cast<uint8>(InputButtons::SHIFT));
	if (comp->mJump)			mInputPacket.Buttons |= (1 << static_cast<uint8>(InputButtons::SPACE));
	if (comp->mAttack)			mInputPacket.Buttons |= (1 << static_cast<uint8>(InputButtons::ATTACK));

	// Convert InputComponent data to SendRequest format
	seq->Type = PKT_Type::C2S_PKT_INPUT;
	seq->SIze = sizeof(C2S_InputPacket);
	
	seq->StoreAs(mInputPacket);


}

void NetSendSystem::QueueGameStart()
{
	mPendingGameStart = true;
	mHasSentGameStart = false;

}

void NetSendSystem::SendSceneChange(SceneId targetScene)
{

	if (targetScene == SceneId::Game)
	{
		UpdateCachedPlayerType();
		gEngine->GetSceneManager().StorePendingPlayerType(mCachedPlayerType);
	}


	C2S_SceneChangePacket changePacket(targetScene);
	SendRequest changeSeq;
	changeSeq.Type = PKT_Type::C2S_SCENE_CHANGE;
	changeSeq.SIze = sizeof(C2S_SceneChangePacket);
	changeSeq.StoreAs(changePacket);
	gSendBuffer.Push(changeSeq);

}

void NetSendSystem::TrySendGameStart()
{
	if (!mPendingGameStart || mHasSentGameStart)
		return;


	UpdateCachedPlayerType();


	cout << "start Game" << endl;
	const uint32 clientId = Network::GetInstance().mClientId;
	C2S_StartGamePacket startPacket;
	startPacket.clientId = clientId;

	cout << "snad Pack Player type" << (int)mCachedPlayerType << endl;

	startPacket.playerType = mCachedPlayerType;
	startPacket.SessionId = clientId;
	startPacket.Sequence = 0;

	SendRequest startSeq;
	startSeq.Type = PKT_Type::C2S_GAME_START;
	startSeq.SIze = sizeof(C2S_StartGamePacket);
	startSeq.StoreAs(startPacket);
	gSendBuffer.Push(startSeq);
	mHasSentGameStart = true;
	mPendingGameStart = false;
}

void NetSendSystem::UpdateCachedPlayerType()
{

	if (!mWorld->HasComponentPool<ChoicePlayerComponent>() )
		return;

	std::vector<Entity> choiceplayerEntities = mWorld->GetEntitiesWithComponent<ChoicePlayerComponent>();
	if (choiceplayerEntities.empty())
		return;

	ChoicePlayerComponent* characterChoice = mWorld->GetComponent<ChoicePlayerComponent>(choiceplayerEntities[0]);

	if (characterChoice)
	{
		//SetCachedPlayerType(characterChoice->mPlayerType);
		mCachedPlayerType = characterChoice->mPlayerType;
		//cout << (int)mCachedPlayerType << endl;
	}
}



