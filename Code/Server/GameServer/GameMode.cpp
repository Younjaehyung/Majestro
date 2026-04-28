#include "pch.h"
#include "GameMode.h"
#include "Scene.h"
#include "World.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "TransformComponent.h"
#include "EnemyComponent.h"
#include "ServerCore.h"


void LobbyGameMode::Initialize()
{

}

void LobbyGameMode::PostUpdate(float deltaTime)
{
}

void LobbyGameMode::SendSceneState(float deltaTime, const std::vector<uint32>& recipients)
{
}

//--------------------------------------------------------------

void WaveGameMode::Initialize()
{

}

void WaveGameMode::PreUpdate(float deltaTime)
{
	mGameTime += deltaTime;



}

void WaveGameMode::PostUpdate(float deltaTime)
{

	if (mScene == nullptr) return;

	/*EntityView view = mScene->GetWorld()->View<MainPlayerComponent>();
	EntityView enemyView = mScene->GetWorld()->View<EnemyComponent>();

	Entity conquestPoint = mConquestPointRect[mWave];
	BoxColliderComponent* conquestPointCol = mScene->GetWorld()->GetComponent<BoxColliderComponent>(conquestPoint);

	mWaveSpawners[0].clear();
	mWaveSpawners[1].clear();

	for(auto player : view)
	{
		MainPlayerComponent* playerComp = mScene->GetWorld()->GetComponent<MainPlayerComponent>(player);
		BoxColliderComponent* playerCol = mScene->GetWorld()->GetComponent<BoxColliderComponent>(player);
		TransformComponent* playerTransform = mScene->GetWorld()->GetComponent<TransformComponent>(player);
		if(playerComp == nullptr || playerCol == nullptr || playerTransform == nullptr) continue;
		
		if(conquestPointCol->mWorldOBB.Intersects(playerCol->mWorldOBB))
		{
			mWaveSpawners[0].push_back(player);
		}


	}

	for (auto enemy : enemyView)
	{
		EnemyComponent* enemyComp = mScene->GetWorld()->GetComponent<EnemyComponent>(enemy);
		BoxColliderComponent* enemyCol = mScene->GetWorld()->GetComponent<BoxColliderComponent>(enemy);
		TransformComponent* enemyTransform = mScene->GetWorld()->GetComponent<TransformComponent>(enemy);
		if(enemyComp == nullptr || enemyCol == nullptr || enemyTransform == nullptr) continue;
		if(conquestPointCol->mWorldOBB.Intersects(enemyCol->mWorldOBB))
		{
			mWaveSpawners[1].push_back(enemy);
		}
	}*/
	mPlayerNum = 0;
	mEnemyNum = 0;

	mScene->GetWorld()->GetEventManager()->Consume<EvConquestPointCaptured>([&](const EvConquestPointCaptured& e) {

		if (e.currentPointsNum != mWave) return;
		mPlayerNum = e.playerNum;
		mEnemyNum = e.enemyNum;
		
	});


	if (mPlayerNum >= mEnemyNum) {

		if (mPlayerNum > mEnemyNum) {
			mWaveTime += deltaTime;
		}
		mWaveInterval = 0.f;
		std::cout << "[ConquestZone] :  PlayerNum: " << mPlayerNum << ", EnemyNum: " << mEnemyNum << ", WaveTime: " << mWaveTime << std::endl;
	}
	else {
		// 웨이브 점령 감소 간격이 최대 간격보다 작으면 간격 증가, 
		// 그렇지 않으면 간격 초기화 및 웨이브 점령 시간 감소
		if (mMaxWaveInterval > mWaveInterval) {
			mWaveInterval += deltaTime;
		}
		else {
			mWaveInterval = 0.f;

			if (mWaveTime > mWaveTime / float(mWaveCheckPoint))
				mWaveTime -= deltaTime;
		}
		mWaveTime = max(0.f, mWaveTime);
	}

	// 웨이브 점령 시간이 웨이브 체크포인트의 비율보다 크면 체크포인트 증가
	// 예: 최대 웨이브 시간이 30초, 체크포인트가 3개인 경우, 웨이브 점령 시간이 10초마다 체크포인트 증가
	 // 웨이브 점령 시간이 10초보다 크면 체크포인트 1 증가, 20초보다 크면 체크포인트 2 증가, 30초보다 크면 체크포인트 3 증가
	// 점령을 안하고 있을시 mWaveTime이 줄어들지만 체크포인트는 유지되어야 하므로, 체크포인트 증가 조건은 mWaveTime이 최대 웨이브 시간의 체크포인트 비율보다 클 때로 설정
	
	if (mWaveTime > mWaveCheckPointTime * float( mWaveCheckPoint) && mWaveCheckPoint < mMaxWaveCheckPoint) {
		mWaveCheckPoint += 1;
	}


	// 웨이브 점령 시간이 최대 웨이브 시간보다 크면 웨이브 증가
	if (mWaveTime > mMaxWaveTime) {
		mWave += 1;
		mWaveTime = 0.f;
		mWaveCheckPoint = 0;
		mWaveInterval = 0.f;
	}

}

void WaveGameMode::SendSceneState(float deltaTime, const std::vector<uint32>& recipients)
{
	S2C_SceneStatePacket pkt{};
	pkt = S2C_SceneStatePacket(0, mGameTime, mWaveCheckPoint, mWave,
		mWaveInterval, mWaveTime, mPlayerNum, mEnemyNum);

	for (uint32 sessionId : recipients)
	{
		mSendReq.SessionId = sessionId;
		mSendReq.Type = S2C_PKT_SCENE_STATE;
		mSendReq.Size = sizeof(S2C_SceneStatePacket);
		mSendReq.StoreAs<S2C_SceneStatePacket>(pkt);
		//	netComp->mIsDirty = false;
		gSendQueue.Push(mSendReq);
	}

}

//--------------------------------------------------------------

void ResultGameMode::Initialize()
{
}

void ResultGameMode::PreUpdate(float deltaTime)
{
}

void ResultGameMode::PostUpdate(float deltaTime)
{
}

void ResultGameMode::SendSceneState(float deltaTime, const std::vector<uint32>& recipients)
{
	// 게임 결과 패킷 전송 (예: 승리/패배, 최종 점수 등)
}