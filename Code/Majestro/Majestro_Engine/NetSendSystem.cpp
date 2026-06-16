#include "pch.h"
#include "NetSendSystem.h"
#include "Engine.h"
#include "Entity.h"
#include "World.h"
#include "Timer.h"

#include "NetEntityComponent.h"
#include "InputManager.h"
#include "MovementComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "LobbyRoomListComponent.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "BeatSystem.h"

#include "SceneManager.h"


NetSendSystem::NetSendSystem(World* world) : System::System(world)
{
	mPhase = SysPhase::Pre;
}


void NetSendSystem::Update(float deltaTime)
{
	TrySendGameStart();
	TrySendScene();                              // 즉시 전송 (이벤트성, TCP)
	TrySendActionEvents();                       // 즉시 전송 (이벤트성, TCP)
	TrySendRoomEvents();                         // Ready/Character 변경 (TCP)
	TrySendRoomBrowserEvents();                  // 방 생성/입장/목록/나가기 (TCP)

	if (mMovementRate.Tick(deltaTime))           // 30Hz 주기 전송 (UDP)
		TrySendMovement();

	if (mSyncRate.Tick(deltaTime))               // 시간 동기 ping (TCP, 4Hz)
		TrySendSync();
}

// 공유 Song Clock
void NetSendSystem::TrySendSync()
{
	// 게임 씬(로컬 플레이어 존재)에서만 동기
	if (!mWorld->HasComponentPool<LocalPlayerComponent>())
		return;

	C2S_SyncPacket pkt{};
	pkt.clientId = Network::GetInstance().mClientId;
	pkt.SendTime = static_cast<double>(TIMER.GetTotalTime());
	SendPacket(pkt);
}

uint32 NetSendSystem::GetCurrentRoomId() const
{
	if (!mWorld->HasComponentPool<LobbyRoomListComponent>())
		return 0;
	auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
	if (entities.empty()) return 0;
	auto* listComp = mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
	return listComp ? listComp->mCurrentRoomId : 0;
}



void NetSendSystem::TrySendGameStart()
{
	if (!mPendingGameStart)
		return;

	if (mHasSentGameStart)
		return;

	const uint32 clientId = Network::GetInstance().mClientId;
	C2S_StartGamePacket startPacket;
	startPacket.clientId    = clientId;
	// 캐릭터는 서버가 RoomState로 가져옴
	startPacket.playerType  = 0;
	startPacket.SessionId   = clientId;

	SendPacket(startPacket);
	mHasSentGameStart = true;
	mPendingGameStart = false;
}



void NetSendSystem::TrySendActionEvents()
{
	if (!mWorld->HasComponentPool<NetEntityComponent>()) return;
	if (!mWorld->HasComponentPool<LocalPlayerComponent>()) return;

	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponents<PlayerMovementComponent, LocalPlayerComponent>();
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

	//std::cout << "Buttons bitmask: " << std::bitset<8>(pkt.Buttons) << std::endl;
	

	// 이전 프레임 대비 변화된 버튼(press/release 모두)이 있을 때만 전송
	const uint32 changed = pkt.Buttons ^ mPrevButtons;
	mPrevButtons = pkt.Buttons;
	if (changed == 0) return;

	pkt.netEntityId = netEnt->mNetEntityId;
	pkt.Yaw         = comp->mCameraRotationY;
	pkt.Pitch       = comp->mCameraRotationX;
	FillCameraFields(pkt.CameraX, pkt.CameraY, pkt.CameraZ,
	                 pkt.CameraDirX, pkt.CameraDirY, pkt.CameraDirZ);

	// 박자 판정 기준
	float songPos = 0.f, beatSec = 0.f;
	if (auto systemManager = mWorld->GetSystemManager())
	{
		if (BeatSystem* beatSystem = systemManager->GetSystem<BeatSystem>())
		{
			songPos = beatSystem->GetSongPosition();	// 입력 순간의 공유 Song Clock 곡 위치
			beatSec = beatSystem->GetBeatSeconds();
		}
	}
	pkt.inputSongPos = songPos;

	SendPacket(pkt);

	// 클라 즉시 1차 판정(예측)
	const uint32 pressed = changed & pkt.Buttons;
	const uint32 actionMask =
		(1u << static_cast<uint8>(InputButtons::ATTACK)) |
		(1u << static_cast<uint8>(InputButtons::SKILL1)) |
		(1u << static_cast<uint8>(InputButtons::SKILL2)) |
		(1u << static_cast<uint8>(InputButtons::RELOAD));
	if ((pressed & actionMask) && beatSec > 0.f)
	{
		uint8 actionButton = static_cast<uint8>(InputButtons::ATTACK);
		if      (pressed & (1u << static_cast<uint8>(InputButtons::ATTACK))) actionButton = static_cast<uint8>(InputButtons::ATTACK);
		else if (pressed & (1u << static_cast<uint8>(InputButtons::SKILL1))) actionButton = static_cast<uint8>(InputButtons::SKILL1);
		else if (pressed & (1u << static_cast<uint8>(InputButtons::SKILL2))) actionButton = static_cast<uint8>(InputButtons::SKILL2);
		else if (pressed & (1u << static_cast<uint8>(InputButtons::RELOAD))) actionButton = static_cast<uint8>(InputButtons::RELOAD);

		const uint8 judgement = ClassifyBeatJudgement(songPos, beatSec);
		mWorld->GetEventManager()->Enqueue(EvBeatJudgement{ judgement, actionButton, true });
		std::cout << "[BeatJudge/predict] btn " << static_cast<int>(actionButton)
		          << " => " << static_cast<int>(judgement) << std::endl;
	}
}

void NetSendSystem::FillCameraFields(float& outPosX, float& outPosY, float& outPosZ,
                                     float& outDirX, float& outDirY, float& outDirZ)
{
	outPosX = outPosY = outPosZ = 0.0f;
	outDirX = outDirY = outDirZ = 0.0f;

	if (!mWorld->HasComponentPool<MainCameraComponent>()) return;
	if (!mWorld->HasComponentPool<TransformComponent>()) return;

	auto cameraEntities = mWorld->GetEntitiesWithComponents<MainCameraComponent, TransformComponent>();
	if (cameraEntities.empty()) return;

	TransformComponent* cameraTransform = mWorld->GetComponent<TransformComponent>(cameraEntities[0]);
	if (!cameraTransform) return;

	Vec3 cameraDirection = cameraTransform->GetLook();
	if (cameraDirection.LengthSquared() > 0.0001f)
		cameraDirection.Normalize();

	outPosX = cameraTransform->mWorldPosition.x;
	outPosY = cameraTransform->mWorldPosition.y;
	outPosZ = cameraTransform->mWorldPosition.z;
	outDirX = cameraDirection.x;
	outDirY = cameraDirection.y;
	outDirZ = cameraDirection.z;
}

void NetSendSystem::TrySendMovement()
{
	if (!mWorld->HasComponentPool<NetEntityComponent>()) return;
	if (!mWorld->HasComponentPool<LocalPlayerComponent>()) return;

	std::vector<Entity> playerEntities = mWorld->GetEntitiesWithComponents<PlayerMovementComponent, LocalPlayerComponent>();
	if (playerEntities.empty()) return;

	Entity playerEntity = playerEntities[0];
	PlayerMovementComponent* comp   = mWorld->GetComponent<PlayerMovementComponent>(playerEntity);
	NetEntityComponent*      netEnt = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (!comp || !netEnt) return;  // 엔티티 할당 전엔 전송 보류

	C2S_MovePacket pkt{};
	pkt.netEntityId = netEnt->mNetEntityId;
	pkt.Seq = ++mMoveSeq;
	pkt.Dt = mMovementRate.Interval;
	pkt.MoveX       = comp->mMovingDirection.x;
	pkt.MoveY       = comp->mJump;
	pkt.MoveZ       = comp->mMovingDirection.z;
	pkt.Yaw         = comp->mCameraRotationY;
	pkt.Pitch       = comp->mCameraRotationX;
	FillCameraFields(pkt.CameraX, pkt.CameraY, pkt.CameraZ,
	                 pkt.CameraDirX, pkt.CameraDirY, pkt.CameraDirZ);

	SendPacket(pkt);
}

void NetSendSystem::TrySendScene()
{
	mWorld->GetEventManager()->Consume<EvNetSceneChange>([this](const EvNetSceneChange& e) {
		mHasSentGameStart = false;
		mPendingGameStart = (e.targetScene == SceneId::FirstGame);
		SendPacket(C2S_SceneChangePacket(e.targetScene));
	});
}

void NetSendSystem::TrySendRoomEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager) return;

	const uint32 roomId = GetCurrentRoomId();   // 현재 방 ID

	eventManager->Consume<EvRoomReadyChanged>([this, roomId](const EvRoomReadyChanged& e) {
		C2S_RoomReadyPacket pkt;
		pkt.roomId = roomId;
		pkt.ready = e.ready ? 1 : 0;
		SendPacket(pkt);
	});

	eventManager->Consume<EvRoomCharacterChanged>([this, roomId](const EvRoomCharacterChanged& e) {
		C2S_RoomCharacterSelectPacket pkt;
		pkt.roomId = roomId;
		pkt.playerType = e.playerType;
		SendPacket(pkt);
	});
}

void NetSendSystem::TrySendRoomBrowserEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager) return;

	eventManager->Consume<EvRoomCreate>([this](const EvRoomCreate&) {
		C2S_RoomCreatePacket pkt;
		SendPacket(pkt);
	});

	eventManager->Consume<EvRoomJoin>([this](const EvRoomJoin& e) {
		C2S_RoomJoinPacket pkt;
		pkt.roomId = e.roomId;
		SendPacket(pkt);
	});

	eventManager->Consume<EvRoomListRequest>([this](const EvRoomListRequest&) {
		C2S_RoomListPacket pkt;
		SendPacket(pkt);
	});

	eventManager->Consume<EvRoomLeave>([this](const EvRoomLeave& e) {
		C2S_RoomLeavePacket pkt;
		pkt.roomId = e.roomId;
		SendPacket(pkt);
	});
}


