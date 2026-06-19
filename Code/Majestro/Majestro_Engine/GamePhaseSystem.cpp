#include "pch.h"
#include "GamePhaseSystem.h"

#include "GameRuleComponent.h"
#include "VfxComponent.h"
#include "TransformComponent.h"
#include "Prefab.h"


void GamePhaseSystem::Initialize()
{

}

void GamePhaseSystem::Update(float deltaTime)
{
	Entity e = mWorld->GetSingletonEntity();

	GameRuleComponent* gameRuleComp = mWorld->GetComponent<GameRuleComponent>(e);
	if (!gameRuleComp) return;
	WavePhaseType currentPhase = static_cast<WavePhaseType>(gameRuleComp->mGamePhase);
	
	ConsumePreparePhase(deltaTime, e);

	switch (currentPhase)
	{
	case WavePhaseType::Prepare:
		// 준비 단계 로직 (예: 웨이브 시작 대기, UI 업데이트 등)
		break;
	case WavePhaseType::Conquest:
	{
		// 점령 구역 마커 VFX 를 활성 구역으로 이동
		GameConquestComponent* conquest = mWorld->GetComponent<GameConquestComponent>(e);
		if (conquest)
			UpdateConquestZoneMarker(conquest);
		break;
	}
	case WavePhaseType::Escort:
	{
		// 호위 단계 로직: 트럭 이동 상태(mMoveState)에 따라 VFX_Escort_Shockwave loop 토글
		GameEscortComponent* escort = mWorld->GetComponent<GameEscortComponent>(e);
		if (escort && escort->mEscortTarget.IsValid())
		{
			if (VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(escort->mEscortTarget))
			{
				if (escort->mMoveState == 1) // 이동 중
				{
					// 무한 loop 재생(끝나면 자동 재시작)
					vfx->mShouldPlay = true;
					vfx->mRestartWhenFinished = true;
				}
				else // 정지
				{
					// 현재 사이클만 끝까지 재생 후 자연 종료
					vfx->mRestartWhenFinished = false;
				}
			}
		}
		break;
	}
	case WavePhaseType::Boss:
		// 보스 단계 로직 (예: 보스 행동 패턴, 체력 관리 등)
		break;
	case WavePhaseType::Fail:
		// 실패 단계 로직 (예: 게임 오버 처리, 리스타트 대기 등)
		break;
	case WavePhaseType::Clear:
		// 클리어 단계 로직 (예: 승리 처리, 다음 스테이지 준비 등)
		break;
	default:
		break;
	}

}

void GamePhaseSystem::ConsumePreparePhase(float deltaTime, Entity e)
{
	mWorld->GetEventManager()->Consume<EvGamePhaseChanged>([this, e](const EvGamePhaseChanged& ev) {
		
		switch (static_cast<WavePhaseType>(ev.prevPhase))
		{
		case WavePhaseType::Prepare:
			// Prepare 전용 네트워크 상태가 다음 Phase에 남지 않도록 제거한다.
			mWorld->RemoveComponent<GamePrepareComponent>(e);
			break;
		case WavePhaseType::Conquest:
			// 점령 단계 종료
			
			// 구역 마커 VFX 끄기 + 싱글톤 정리
			StopConquestZoneMarker();
			mWorld->RemoveComponent<GameConquestComponent>(e);
			break;
		case WavePhaseType::Escort:
			// 호위 단계 로직 (예: 호위 대상 이동, 적 스폰 등)
			mWorld->RemoveComponent<GameEscortComponent>(e);
			break;
		case WavePhaseType::Boss:
			// 보스 단계 로직 (예: 보스 행동 패턴, 체력 관리 등)
			break;
		case WavePhaseType::Fail:
			// 실패 단계 로직 (예: 게임 오버 처리, 리스타트 대기 등)
			break;
		case WavePhaseType::Clear:
			// Clear 결과 상태는 Clear Phase에서만 유지한다.
			mWorld->RemoveComponent<GameClearComponent>(e);
			break;
		default:
			break;
		}

		switch (static_cast<WavePhaseType>(ev.newPhase))
		{
		case WavePhaseType::Prepare:
			// Prepare 패킷 수신 전에 UI가 접근해도 안전하도록 상태를 생성한다.
			if (!mWorld->GetComponent<GamePrepareComponent>(e))
				mWorld->AddComponent<GamePrepareComponent>(e);
			break;
		case WavePhaseType::Conquest:
			// 점령 단계 로직 (예: 점령 시간 감소, 점령 상태 체크 등)
			mWorld->AddComponent<GameConquestComponent>(e);
			break;
		case WavePhaseType::Escort:
			// 호위 단계 로직 (예: 호위 대상 이동, 적 스폰 등)
			mWorld->AddComponent<GameEscortComponent>(e);
			break;
		case WavePhaseType::Boss:
			// 보스 단계 로직 (예: 보스 행동 패턴, 체력 관리 등)
			break;
		case WavePhaseType::Fail:
			// 실패 단계 로직 (예: 게임 오버 처리, 리스타트 대기 등)
			break;
		case WavePhaseType::Clear:
			// Clear 패킷 수신 전에 UI가 접근해도 안전하도록 상태를 생성한다.
			if (!mWorld->GetComponent<GameClearComponent>(e))
				mWorld->AddComponent<GameClearComponent>(e);
			break;
		default:
			break;
		}

		});
}

Entity GamePhaseSystem::EnsureConquestMarker()
{
	// 첫 Conquest 진입 시에만 생성하고 이후 씬 동안 재사용
	if (!mConquestMarker.IsValid())
	{
		InputCommand ctx{};
		mConquestMarker = AreaConquestPrefab::Build(mWorld, ctx);
	}
	return mConquestMarker;
}

void GamePhaseSystem::UpdateConquestZoneMarker(GameConquestComponent* conquest)
{
	const uint8 zoneId = static_cast<uint8>(conquest->mActiveZoneId);

	// 활성 구역 정보가 아직 안 옴
	if (zoneId == 0)
	{	// 마커 꺼둠
		StopConquestZoneMarker();
		return;
	}

	Entity marker = EnsureConquestMarker();
	VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(marker);
	TransformComponent* tr = mWorld->GetComponent<TransformComponent>(marker);
	if (vfx == nullptr || tr == nullptr)
		return;

	// 활성 구역이 바뀌면
	// 이전 위치의 VFX 는 끄고 새 구역 위치에서 재시작한다.
	if (zoneId != mShownConquestZoneId)
	{
		tr->mLocalPosition = conquest->mActiveZonePos;
		tr->mWorldPosition = conquest->mActiveZonePos;
		tr->mWorldMatrix = Matrix::CreateTranslation(conquest->mActiveZonePos);

		// 수정: 기존 VFX 핸들을 제거하면 mIsPlaying 상태와 불일치하여
		// 다음 점령지에서 새 VFX가 재생되지 않으므로 핸들을 유지한 채 위치만 갱신한다.
		// EffectPass가 매 프레임 Transform 위치를 Effekseer 핸들에 반영한다.
		vfx->mFinished = false;
		mShownConquestZoneId = zoneId;
	}

	// 오픈 ~ 마감까지 계속 loop 재생
	vfx->mShouldPlay = true;
	vfx->mRestartWhenFinished = true;
}

void GamePhaseSystem::StopConquestZoneMarker()
{
	mShownConquestZoneId = 0;

	if (!mConquestMarker.IsValid())
		return;

	if (VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(mConquestMarker))
	{
		// 종료
		vfx->mShouldPlay = false;
		vfx->mRestartWhenFinished = false;
	}
}
