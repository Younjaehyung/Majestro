#include "pch.h"
#include "World.h"
#include "ResourceManager.h"
#include "LightComponent.h"
#include "TransformComponent.h"
#include "CameraComponent.h"


void LightComponent::SetLightDirection(Vec3 direction)
{
	direction.Normalize();

	mLightInfo.Direction = direction;
}

Entity LightFactory::CreateLight(shared_ptr<World> world, LIGHT_TYPE type , LightComponent& l)
{
	Entity entity = world->CreateEntity();

	CameraComponent c{};
	TransformComponent t{};


	c.SetScale(1.f);
	c.SetFar(1000.f);
	c.SetWidth(4096);
	c.SetHeight(4096);


	t.mLocalPosition = { Vec3(l.mLightInfo.Position) };
	t.LookAt(Vec3(l.mLightInfo.Direction));

	world->AddComponent<LightComponent>(entity, l);
	world->AddComponent<TransformComponent>(entity,t );
	world->AddComponent<CameraComponent>(entity, c);

	return entity;
}
