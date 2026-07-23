#include "pch.h"
#include "TransformSystem.h"
#include "TransformComponent.h"
#include "TerrainComponent.h"
#include "GravityComponent.h"

TransformSystem::TransformSystem(World* world) : System(world)
{
	
}



void TransformSystem::Update(float dt) {
	auto view = mWorld->View<TransformComponent>();
	for (Entity entity : view) {

		TransformComponent* transformComponent = mWorld->GetComponent<TransformComponent>(entity);
		
		if(transformComponent -> mIsStatic)
			continue;
		Matrix result = transformComponent->FinalUpdate();
		const uint64 parentWorldRevision = transformComponent->GetWorldRevision();


		if (!transformComponent->mChild.empty()) {
			for (Entity& e : transformComponent->mChild) {
				TransformComponent* childComp = mWorld->GetComponent<TransformComponent>(e);
				if (childComp) {

					childComp->FinalUpdate(result, parentWorldRevision);
				}
			}
		}

	}

}
