#include "pch.h"
#include "GameCore.h"
#include "ServerCore.h"

void GameCore::Initialize()
{
	mAIManager = std::make_unique<AIManager>();
	mResourceManager = std::make_unique<ResourceManager>();
	mSceneManager = std::make_unique<SceneManager>();

	mRoomManager = std::make_unique<RoomManager>();
	mSceneManager->SetRoomManager(mRoomManager.get());


	// 핸들러/노티파이어가 파싱·직렬화를 전담
	mRoomNotifier = std::make_unique<RoomNotifier>();
	mRoomNotifier->SetRoomManager(mRoomManager.get());
	mRoomManager->SetNotifier(mRoomNotifier.get());

	mRoomPacketHandler = std::make_unique<RoomPacketHandler>(mRoomManager.get(), mRoomNotifier.get());
	mScenePacketHandler = std::make_unique<ScenePacketHandler>(mSceneManager.get());
	mPacketRouter = std::make_unique<PacketRouter>(mSceneManager.get(), mRoomPacketHandler.get(), mScenePacketHandler.get());
}

void GameCore::Start()
{
	MJLOG_INFO(Startup, "게임 데이터 로드 시작");
	LoadGameData();
	MJLOG_INFO(Startup, "게임 데이터 로드 완료");
}

void GameCore::Update(float deltaTime)
{

	InputCommand command;
	while (gRecvQueue.Pop(command))
	{
		mPacketRouter->Dispatch(command);
	}

	mSceneManager->Update(deltaTime);
}

void GameCore::LoadGameData()
{
	mAIManager->Initialize();
	mResourceManager->Initialize();
	mRoomManager->Initialize();
	mSceneManager->Initialize();
}

void GameCore::UpdateGameLogic(float deltaTime)
{
}
