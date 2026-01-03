#include "pch.h"
#include "BeatSystem.h"
#include "BeatComponent.h"

BeatSystem::BeatSystem(World* world) : System(world)
{
}

void BeatSystem::Initialize()
{
}

void BeatSystem::Update(float dt)
{

	mSeconds += dt;

	cout << "time :" << mSeconds << endl;
	mBeat = (int)(mSeconds / (60.0f / mBpm));
	mBeat %= (int)mBpm;
	cout << "Beat :" << mBeat << endl;


	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<BeatComponent>() };

	
	//TestUpdate(dt);
	for (auto& entity : entitys) {
		//CameraComponent* cameraComponent = mWorld->GetComponent<CameraComponent>(entity);
		//TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);

		//transformComponent->FinalUpdate();
		//cameraComponent->FinalUpdate(transformComponent->GetLocalToWorldMatrix().Invert());
	}
	
}