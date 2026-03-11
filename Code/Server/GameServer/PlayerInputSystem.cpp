#include "pch.h"
#include "PlayerInputSystem.h"
#include <chrono>

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
#include "ColliderComponent.h"
#include <unordered_set>

namespace
{
	constexpr float kDegToRad = 0.01745329251994329577f;

	Vec3 GetCameraForwardFromInput(const InputComponent& input)
	{
		const float yawRad = input.Yaw * kDegToRad;
		const float pitchRad = -input.Pitch * kDegToRad;

		const float cosPitch = std::cos(pitchRad);
		Vec3 forward;
		forward.x = std::sin(yawRad) * cosPitch;
		forward.y = std::sin(pitchRad);
		forward.z = std::cos(yawRad) * cosPitch;

		if (forward.LengthSquared() <= 0.0001f)
			return Vec3::Forward;

		forward.Normalize();
		return forward;
	}

	BulletType ResolveBulletType(uint8 playerType, InputButtons actionButton)
	{
		switch (playerType)
		{
		case 0:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::DrumAttack;
			case InputButtons::SKILL1: return BulletType::DrumSkill1;
			case InputButtons::SKILL2: return BulletType::DrumSkill2;
			default: return BulletType::Default;
			}
		case 1:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::BaseAttack;
			case InputButtons::SKILL1: return BulletType::BaseSkill1;
			case InputButtons::SKILL2: return BulletType::BaseSkill2;
			default: return BulletType::Default;
			}
		default:
			switch (actionButton)
			{
			case InputButtons::ATTACK: return BulletType::GuitarAttack;
			case InputButtons::SKILL1: return BulletType::GuitarSkill1;
			case InputButtons::SKILL2: return BulletType::GuitarSkill2;
			default: return BulletType::Default;
			}
		}
	}

}

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
		mainPlayerComponent->mPlayerMovingDir.x = inputComp->MoveZ;
		mainPlayerComponent->mPlayerMovingDir.y = inputComp->MoveX;
		if (inputComp->MoveX == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunRightState::Instance());
		}
		if (inputComp->MoveX == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunLeftState::Instance());
		}
		if (inputComp->MoveZ == 1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunForwardState::Instance());
		}
		if (inputComp->MoveZ == -1) {
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, RunBackwardState::Instance());
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

			//DrumAttack
			ActivateBulletAndNotify(e, ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK));
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Attack1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::RELOAD)) {//attack
			//std::cout << "attack!!!" << std::endl;
			//ActivateBulletAndNotify(e, ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::ATTACK));
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, ReRoadState::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SPECIAL)) {//mRhythm change
			//std::cout << "special" << std::endl;
			//mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, SpecialState::Instance());
		}


		if (inputComp->IsButtonPressed(InputButtons::SKILL1)) {
			//std::cout << "skill1" << std::endl;
			ActivateBulletAndNotify(e, ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::SKILL1));
			mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill1State::Instance());
		}
		if (inputComp->IsButtonPressed(InputButtons::SKILL2)) {
			if (mainPlayerComponent->mPlayerType == 1)
			{
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, DashState::Instance());
			}
			else
			{
				ActivateBulletAndNotify(e, ResolveBulletType(mainPlayerComponent->mPlayerType, InputButtons::SKILL2));
				mainPlayerComponent->mFsm.ChangeState(mainPlayerComponent, Skill2State::Instance());
			}
		}


		if (inputComp->IsMousePressed(InputMouse::LEFT)) {
			

			//screen move
			//movementComponent->mCameraRotationX += (float)inputComp->Pitch * dt * mDPI;
			//movementComponent->mCameraRotationY += (float)inputComp->Yaw * dt * mDPI;

		}

		mainPlayerComponent->Update(dt);

	}

	
}

void PlayerInputSystem::ActivateBulletAndNotify(Entity playerEntity, BulletType bulletType)
{
	if (false == mWorld->HasComponentPool<BulletComponent>())
		return;

	TransformComponent* playerTransform = mWorld->GetComponent<TransformComponent>(playerEntity);
	NetEntityComponent* playerNetComp = mWorld->GetComponent<NetEntityComponent>(playerEntity);
	InputComponent* inputComp = mWorld->GetComponent<InputComponent>(playerEntity);
	if (playerTransform == nullptr || playerNetComp == nullptr || inputComp == nullptr)
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

		Vec3 direction = GetCameraForwardFromInput(*inputComp);
		const BulletStat bulletStat = GetBulletStat(bulletType);

		bulletTransform->mWorldPosition = playerTransform->mWorldPosition + direction * 3.0f + Vec3(0.f, 90.f, 0.f);
		bulletTransform->mLocalPosition = bulletTransform->mWorldPosition;
		bulletTransform->mLocalScale = Vec3(bulletStat.Size, bulletStat.Size, bulletStat.Size);
		bulletTransform->mMovingVector = direction * bulletStat.Speed;

		if (BoxColliderComponent* bulletCollider = mWorld->GetComponent<BoxColliderComponent>(bulletEntity))
		{
			const float halfSize = bulletStat.Size * 0.5f;
			bulletCollider->SetBox(Vec3(halfSize, halfSize, halfSize), Vec3::Zero);
		}

		const uint16 generation = static_cast<uint16>(bulletComp->mGeneration + 1);
		bulletComp->mPenetrationCount = bulletStat.PenetrationCount;
		bulletComp->Activate(bulletType, playerNetComp->mNetEntityId, static_cast<uint32>(bulletNetComp->mNetEntityId), generation, direction, bulletStat.Speed, bulletStat.LifeTime, bulletStat.Damage, bulletStat.KnockbackDistance);
		bulletComp->mIsMeleeAttack = bulletStat.IsMeleeAttack;

		mWorld->RegisterActiveBullet(bulletEntity);

		S2C_BulletActivatePacket bulletPacket{};
		bulletPacket.SendTime = std::chrono::duration<double>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		bulletPacket.ownerNetEntityId = playerNetComp->mNetEntityId;
		bulletPacket.bulletNetEntityId = bulletNetComp->mNetEntityId;
		bulletPacket.bulletType = static_cast<uint8>(bulletType);
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