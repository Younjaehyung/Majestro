#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "EnginePch.h"
#include "Entity.h"
#include "World.h"

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


void NetSendSystem::UpdateCachedPlayerType()
{

	if (!mWorld->HasComponentPool<ChoicePlayerComponent>())
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

void NetSendSystem::Update(float deltaTime)
{
	UpdateCachedPlayerType();
	TrySendGameStart();
	TrySendScene();                              // 즉시 전송 (이벤트성, TCP)
	TrySendActionEvents();                       // 즉시 전송 (이벤트성, TCP)

	if (mMovementRate.Tick(deltaTime))           // 30Hz 주기 전송 (UDP)
		TrySendMovement();
}



void NetSendSystem::TrySendGameStart()
{

	UpdateCachedPlayerType();

	cout << "start Game" << endl;
	cout << "send Pack Player type " << (int)mCachedPlayerType << endl;

	const uint32 clientId = Network::GetInstance().mClientId;
	C2S_StartGamePacket startPacket;
	startPacket.clientId    = clientId;
	startPacket.playerType  = mCachedPlayerType;
	startPacket.SessionId   = clientId;

	SendPacket(startPacket);

}



void NetSendSystem::TrySendActionEvents()
{
	if (!mWorld->HasComponentPool<NetEntityComponent>()) return;

	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponent<PlayerMovementComponent>();
	if (playerEntities.empty()) return;

	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
	NetEntityComponent* netEnt = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (!comp || !netEnt) return;

	// 이번 프레임 눌린 버튼 수집
	C2S_ActionPacket pkt{};
	if (comp->mDash)    pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::SHIFT));
	if (comp->mJump)    pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::SPACE));
	if (comp->mAttack)  pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::ATTACK));
	if (comp->mSkill1)  pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::SKILL1));
	if (comp->mSkill2)  pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::SKILL2));
	if (comp->mReload)  pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::RELOAD));
	if (comp->mSpecial) pkt.Buttons |= (1 << static_cast<uint8>(InputButtons::SPECIAL));

	std::cout << "Buttons bitmask: " << std::bitset<8>(pkt.Buttons) << std::endl;
	

	// 이전 프레임 대비 변화된 버튼(press/release 모두)이 있을 때만 전송
	const uint32 changed = pkt.Buttons ^ mPrevButtons;
	mPrevButtons = pkt.Buttons;
	if (changed == 0) return;

	pkt.netEntityId = netEnt->mNetEntityId;
	pkt.Yaw         = comp->mCameraRotationY;
	pkt.Pitch       = comp->mCameraRotationX;

	SendPacket(pkt);
}

void NetSendSystem::TrySendMovement()
{
	if (!mWorld->HasComponentPool<NetEntityComponent>()) return;

	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponent<PlayerMovementComponent>();
	if (playerEntities.empty()) return;

	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp   = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
	NetEntityComponent*      netEnt = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (!comp || !netEnt) return;  // 엔티티 할당 전엔 전송 보류

	C2S_MovePacket pkt{};
	pkt.netEntityId = netEnt->mNetEntityId;
	pkt.MoveX       = comp->mMovingDirection.x;
	pkt.MoveY       = comp->mJump;
	pkt.MoveZ       = comp->mMovingDirection.z;
	pkt.Yaw         = comp->mCameraRotationY;
	pkt.Pitch       = comp->mCameraRotationX;

	SendPacket(pkt);
}

void NetSendSystem::TrySendScene()
{
	mWorld->GetEventManager()->Consume<EvNetSceneChange>([this](const EvNetSceneChange& e) {
		UpdateCachedPlayerType();
		SendPacket(C2S_SceneChangePacket(e.targetScene));
	});
}


