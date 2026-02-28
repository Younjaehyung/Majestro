#include "pch.h"
#include "PlayerInputSystem.h"

#include "PlayerSystem.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "InputManager.h"
#include "TransformSystem.h"
#include "TerrainComponent.h"
#include "BeatComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "NetEntityComponent.h"
#include "ServerCore.h"
#include "BulletComponent.h"
#include "MovementSystem.h"
#include <unordered_set>

PlayerInputSystem::PlayerInputSystem(World* world) : System(world)
{
}

void PlayerInputSystem::Initialize()
{
}

void PlayerInputSystem::Update(float dt)
{
	if (false == mWorld->HasComponentPool<PlayerMovementComponent>())return;

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<PlayerMovementComponent>() };

	for (auto& e : entitys) {
		MainPlayerComponent* mainPlayerComponent = mWorld->GetComponent<MainPlayerComponent>(e);
		BeatComponent* beatComponent = mWorld->GetComponent<BeatComponent>(e);
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(e);
		//NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(e);

		if (inputComp->MoveX ==0 && inputComp->MoveZ == 0) {
			mainPlayerComponent->mSpeed = 0.f;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, IdleState::Instance());
		}
		else {
			if (mainPlayerComponent->mDash) mainPlayerComponent->mSpeed = mainPlayerComponent->mDashSpeed;
			else mainPlayerComponent->mSpeed = mainPlayerComponent->mRunSpeed;
		}
		

		//movementComponent->mMovingDirection = { 0,0,0 };

		if (inputComp->MoveX == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveZ == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveZ == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}
		if (inputComp->MoveX == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, WalkState::Instance());
		}



		if (inputComp->IsButtonPressed(InputButtons::SPACE)) {
			if (beatComponent->mBouns) cout << "Hit Beat!" << endl;
			else cout << "fail" << endl;

			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, JumpState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SHIFT)) {
			cout << "dash" << endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::ATTACK)) {//attack 
			std::cout << "attack!!!" << std::endl;
			ActivateBulletAndNotify(e);
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//attack 
			std::cout << "special" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}


		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			std::cout << "skill1" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			std::cout << "skill2" << std::endl;
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
		}


		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}

void PlayerInputSystem::ActivateBulletAndNotify(Entity playerEntity)
{
	if (false == mWorld->HasComponentPool<BulletComponent>())
		return;

	TransformComponent* playerTransform = mWorld->GetComponent<TransformComponent>(playerEntity);
	NetEntityComponent* playerNetComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	if (playerTransform == nullptr || playerNetComp == nullptr)
		return;

	auto bulletEntities = mWorld->GetEntitiesWithComponents<BulletComponent, TransformComponent, NetEntityComponent>();
	for (auto bulletEntity : bulletEntities)
	{
		BulletComponent* bulletComp = mWorld->GetComponent<BulletComponent>(bulletEntity);
		if (bulletComp == nullptr || bulletComp->mIsActive)
			continue;

		TransformComponent* bulletTransform = mWorld->GetComponent<TransformComponent>(bulletEntity);
		NetEntityComponent* bulletNetComp = mWorld->GetComponent<NetEntityComponent>(bulletEntity);
		if (bulletTransform == nullptr || bulletNetComp == nullptr)
			continue;

		Vec3 direction = playerTransform->GetLook();
		if (direction.LengthSquared() <= 0.0001f)
			direction = Vec3::Forward;
		direction.Normalize();

		bulletTransform->mWorldPosition = playerTransform->mWorldPosition + direction * 2.0f + Vec3(0.f, 1.f, 0.f);
		bulletTransform->mLocalPosition = bulletTransform->mWorldPosition;
		bulletTransform->mMovingVector = direction * bulletComp->mSpeed;

		const uint16 generation = static_cast<uint16>(bulletComp->mGeneration + 1);
		bulletComp->Activate(BulletType::Default, playerNetComp->mNetEntityId, static_cast<uint32>(bulletNetComp->mNetEntityId), generation, direction, bulletComp->mSpeed, bulletComp->mLifeTime, bulletComp->mDamage);

		if (auto movementSystem = mWorld->GetSystemManager()->GetSystem<MovementSystem>())
			movementSystem->RegisterActiveBullet(bulletEntity);

		S2C_BulletActivatePacket bulletPacket{};
		bulletPacket.ownerNetEntityId = playerNetComp->mNetEntityId;
		bulletPacket.bulletNetEntityId = bulletNetComp->mNetEntityId;
		bulletPacket.x = bulletTransform->mWorldPosition.x;
		bulletPacket.y = bulletTransform->mWorldPosition.y;
		bulletPacket.z = bulletTransform->mWorldPosition.z;
		bulletPacket.dirX = direction.x;
		bulletPacket.dirY = direction.y;
		bulletPacket.dirZ = direction.z;
		bulletPacket.speed = bulletComp->mSpeed;

		auto recipients = CollectPlayerSessions();
		for (uint32 sessionId : recipients)
		{
			SendRequest request{ sessionId, PKT_Type::S2C_PKT_BULLET_ACTIVATE, sizeof(S2C_BulletActivatePacket) };
			request.StoreAs<S2C_BulletActivatePacket>(bulletPacket);
			gSendQueue.Push(request);
		}

		return;
	}
}

std::vector<uint32> PlayerInputSystem::CollectPlayerSessions() const
{
	if (false == mWorld->HasComponentPool<NetEntityComponent>())
		return {};

	std::unordered_set<uint32> sessionSet;
	auto players = mWorld->GetEntitiesWithComponents<NetEntityComponent, MainPlayerComponent>();
	sessionSet.reserve(players.size());

	for (auto entity : players)
	{
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp == nullptr || netComp->mSessionId == 0)
			continue;

		sessionSet.insert(netComp->mSessionId);
	}

	return std::vector<uint32>(sessionSet.begin(), sessionSet.end());
}