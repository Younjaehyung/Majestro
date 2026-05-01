#include "pch.h"
#include "GamePhase.h"
#include "GameCore.h"
#include "ResourceManager.h"	

#include "GameRuleComponent.h"
#include "PlayerComponent.h"
#include "ColliderComponent.h"
#include "TransformComponent.h"
#include "EnemyComponent.h"
#include "PayloadPathData.h"
#include "PathLoadComponent.h"
#include "NetEntityComponent.h"

#include "Prefab.h"




void PreparePhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mGameRuleEntity = mode.GetGameRuleEntity();
	auto& state = mWorld->AddComponent<GameRuleComponent>(mGameRuleEntity);
}

void PreparePhase::Exit(WaveGameMode& mode)
{
	//mWorld->RemoveComponent<GameRuleComponent>(mGameRuleEntity);
}

void PreparePhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void PreparePhase::PostUpdate(float dt, WaveGameMode& mode)
{
	// 준비 단계에서는 플레이어가 준비를 완료했는지 체크하는 로직
	// 예: 모든 플레이어가 준비 상태가 되었는지 확인하고, 준비가 완료되면 mIsCompleted를 true로 설정하여 다음 단계로 전환
}

////--------------------------------------------------------------



void ConquestPhase::Enter(WaveGameMode& mode)
{
	mWorld = mode.GetScene()->GetWorld();
	mGameRuleEntity = mode.GetGameRuleEntity();
	GameRuleComponent* ruleComp = mWorld->GetComponent<GameRuleComponent>(mGameRuleEntity);


	mWorld->AddComponent<GameConquestComponent>(mGameRuleEntity);
	ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Conquest);
}

void ConquestPhase::Exit(WaveGameMode& mode)
{

	mWorld->RemoveComponent<GameConquestComponent>(mGameRuleEntity);

}

void ConquestPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void ConquestPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	int playerNum = 0;
	int enemyNum = 0;

	GameConquestComponent* ruleComp = mWorld->GetComponent<GameConquestComponent>(mGameRuleEntity);


	mWorld->GetEventManager()->Consume<EvConquestPointCaptured>([&](const EvConquestPointCaptured& e) {

		if (e.currentPointsNum != ruleComp->mWave) return;
		playerNum = e.playerNum;
		enemyNum = e.enemyNum;

		});


	if (playerNum >= enemyNum) {

		if (playerNum > enemyNum) {
			ruleComp->mWaveTime += dt;
		}
		ruleComp->mWaveInterval = 0.f;
		std::cout << "[ConquestZone] :  PlayerNum: " << playerNum << ", EnemyNum: " << enemyNum << ", WaveTime: " << ruleComp->mWaveTime << std::endl;
	}
	else {
		// 웨이브 점령 감소 간격이 최대 간격보다 작으면 간격 증가, 
		// 그렇지 않으면 간격 초기화 및 웨이브 점령 시간 감소
		if (ruleComp->mMaxWaveInterval > ruleComp->mWaveInterval) {
			ruleComp->mWaveInterval += dt;
		}
		else {
			ruleComp->mWaveInterval = 0.f;

			if (ruleComp->mWaveTime > ruleComp->mWaveTime / float(ruleComp->mWaveCheckPoint))
				ruleComp->mWaveTime -= dt;
		}
		ruleComp->mWaveTime = max(0.f, ruleComp->mWaveTime);
	}

	ruleComp->mPlayerNum = playerNum;
	ruleComp->mEnemyNum = enemyNum;
	


	if (ruleComp->mWaveTime > ruleComp->mWaveCheckPointTime * float(ruleComp->mWaveCheckPoint) && ruleComp->mWaveCheckPoint < ruleComp->mMaxWaveCheckPoint) {
		ruleComp->mWaveCheckPoint += 1;
	}


	// 웨이브 점령 시간이 최대 웨이브 시간보다 크면 웨이브 증가
	if (ruleComp->mWaveTime > ruleComp->mMaxWaveTime) {
		ruleComp->mWave += 1;
		ruleComp->mWaveTime = 0.f;
		ruleComp->mWaveCheckPoint = 0;
		ruleComp->mWaveInterval = 0.f;
	}

}
////--------------------------------------------------------------

EscortPhase::EscortPhase(uint8 routeId) : mRouteId(routeId) 
{
	switch (routeId)
	{
	case 1:
		mEscortPath = RESOURCEMANAGER.Get<PayloadPathData>(L"BP_Payroad_path_C_1_PayloadPath.json");
		break;
	}

}


void EscortPhase::Enter(WaveGameMode& mode)
{
	World* world = mode.GetScene()->GetWorld().get();
	Entity rule = mode.GetGameRuleEntity();
	GameRuleComponent* ruleComp = mWorld->GetComponent<GameRuleComponent>(rule);
	ruleComp->mGamePhase = static_cast<uint8>(WavePhaseType::Escort);





	auto& state = world->AddComponent<GameEscortComponent>(rule);
	state.mRouteId = mRouteId;

	Entity mEscortTarget = world->CreateEntity();
	if(mEscortTarget.IsValid())
	{
		GameEscortComponent* escortComp = world->GetComponent<GameEscortComponent>(rule);
		escortComp->mEscortTarget = mEscortTarget;
		world->AddComponent<TransformComponent>(mEscortTarget);
		world->AddComponent<NetEntityComponent>(mEscortTarget, world, mEscortTarget);

		// 호위 대상에 경로 추종 컴포넌트 부착 — PathFollowSystem 이 매 프레임 위치/회전 갱신
		PathLoadComponent& pathComp = world->AddComponent<PathLoadComponent>(mEscortTarget);
		pathComp.mPathData         = mEscortPath;
		pathComp.mBaseSpeed        = 200.f; 
		pathComp.mCurrentDistance  = 0.f;
		pathComp.mPreviousDistance = 0.f;
		pathComp.mActive           = true;

		auto& inter = world->AddComponent<InteractableComponent>(mEscortTarget);
		inter.mKind = InteractableKind::EscortZone;
		inter.mShape = InteractableShape::Sphere;
		inter.mRadius = state.mEscortRange;       // 500cm
		inter.mIgnoreY = true;
		inter.mTargetMask = InteractableTarget_All;
		inter.mCooldown = 0.f; 
		inter.mActive = true;
	}

}

void EscortPhase::Exit(WaveGameMode& mode)
{
	mWorld->RemoveComponent<GameEscortComponent>(mode.GetGameRuleEntity());
}

void EscortPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void EscortPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	Entity rule = mode.GetGameRuleEntity();
	GameEscortComponent* ruleComp = mWorld->GetComponent<GameEscortComponent>(rule);
	if (!ruleComp)
		return;

	ruleComp->mEscortTime += dt;

	Entity escortTarget = ruleComp->mEscortTarget;
	TransformComponent* targetTr = mWorld->GetComponent<TransformComponent>(escortTarget);

	mWorld->GetEventManager()->Consume<EvEscortPointCaptured>([&](const EvEscortPointCaptured& e) {

		ruleComp->mPlayerNum = e.playerNum;
		ruleComp->mEnemyNum = e.enemyNum;

	});

	PathLoadComponent* pathComp = mWorld->GetComponent<PathLoadComponent>(escortTarget);
	pathComp->mPaused = (ruleComp->mEnemyNum == 0 && ruleComp->mPlayerNum > 0) ? false : true;


	ruleComp->mEscortProgress = pathComp->mCurrentDistance / pathComp->mTotalDistance;

	if (ruleComp->mEscortProgress >= 1.f)
		mIsCompleted = true;
	
	// GameEscortComponent의 상태를 업데이트하거나, 플레이어와 호위 대상의 위치를 체크하여 호위 성공/실패 여부를 판단하는 로직을 구현할 수 있습니다.
}

////--------------------------------------------------------------



void BossPhase::Enter(WaveGameMode& mode)
{
}

void BossPhase::Exit(WaveGameMode& mode)
{
}

void BossPhase::PreUpdate(float dt, WaveGameMode& mode)
{
}

void BossPhase::PostUpdate(float dt, WaveGameMode& mode)
{
	// 보스의 체력이나 행동 패턴을 체크하여 보스가 처치되었는지 판단하는 로직을 구현할 수 있습니다.
}