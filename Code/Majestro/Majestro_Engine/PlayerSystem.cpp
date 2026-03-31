#include "pch.h"
#include "PlayerSystem.h"
#include "Engine.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "RenderComponent.h"
#include "MovementComponent.h"


PlayerSystem::PlayerSystem(World* world) : System(world)
{
}

void PlayerSystem::Initialize()
{
}

void PlayerSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<MainPlayerComponent>())return;
	if (false == mWorld->HasComponentPool<LocalPlayerComponent>())return;

	/*std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<ControllerComponent, TransformComponent>() };
	if (entitys.empty())return;
	MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(entitys[0]);
	mainPlayerComponent->Update(dt);*/

	std::vector<Entity> playerEntitys= mWorld->GetEntitiesWithComponent<CameraTypeComponent>() ;
	for (auto& entity : playerEntitys) {

		CameraTypeComponent* mainPlayer = mWorld->GetComponent<CameraTypeComponent>(entity);
		CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(mainPlayer->mTargetID);
		

		//if(mainPlayerComponent->mStatePacket == S_Attack1)
		//{
		//	mainPlayerComponent->mShakeCameraTime += dt;


		//	cameraComponent->mFov = lerp(cameraComponent->mFov, (103.f / 2.0f) + 0.2f, mainPlayerComponent->mDashTime);
		//	
		//}
		//else
		//{
		//	mainPlayerComponent->mShakeCameraTime = 0.f;
		//	cameraComponent->
		//}



		if(mainPlayerComponent->mStatePacket == S_Dash)
		{
			mainPlayerComponent->mDashTime += dt;
			cameraComponent->mFov = lerp(cameraComponent->mFov, (103.f / 2.0f) + 0.2f, mainPlayerComponent->mDashTime);

		}
		else
		{
			mainPlayerComponent->mDashTime = 0.f;
			cameraComponent->mFov = 103.f / 2.0f;
		}

		
		// 공격 상태 진입 시 카메라 쉐이크 트리거
		if (mainPlayerComponent->mStatePacket != mainPlayerComponent->mPrevStatePacket)
		{
			uint32 entityID = entity.GetID();
			int32  currState = mainPlayerComponent->mStatePacket;

			//mainPlayerComponent->mPlayerType = 0 1 2


			// TriggerShake(float magnitude, float duration, float frequency)
			switch (mainPlayerComponent->mPlayerType) 
			{
			case 0:
				if (currState)
				{
					bool bAttackState = (currState == S_Attack1 || currState == S_Attack2
						|| currState == S_Skill1 || currState == S_Skill2
						|| currState == S_Special);
					if (bAttackState)
						mainPlayer->TriggerShake(1.5f, 0.05f, 20.f);
					std::cout << "ATTACK!@" << std::endl;
				}
				break;
			case 1:
				if (currState)
				{
					bool bAttackState = (currState == S_Attack1 || currState == S_Attack2
						|| currState == S_Skill1 || currState == S_Skill2
						|| currState == S_Special);
					if (bAttackState)
						mainPlayer->TriggerShake(0.5f, 0.05f, 20.f);

				}
				break;
			case 2: 
				if (currState)
				{
					bool bAttackState = (currState == S_Attack1 || currState == S_Attack2
						|| currState == S_Skill1 || currState == S_Skill2
						|| currState == S_Special);
					if (bAttackState)
						mainPlayer->TriggerShake(0.5f, 0.05f, 20.f);

				}
				break;
			}
		}

	}
}
