#include "pch.h"
#include "Engine.h"
#include "World.h"
#include "ResourceManager.h"
#include "LightComponent.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "CameraComponent.h"


void LightComponent::SetLightDirection(Vec3 direction)
{
	direction.Normalize();

	mLightInfo.Direction = direction;

	//GetTransform()->LookAt(direction);
}
//
//void LightComponent::SetLightType(LIGHT_TYPE type)
//{
//	mLightInfo.LightType = static_cast<int32>(type);
//
//	switch (type)
//	{
//	case LIGHT_TYPE::DIRECTIONAL_LIGHT:
//		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Rectangle");
//		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"DirLight");
//
//		break;
//	case LIGHT_TYPE::POINT_LIGHT:
//		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");
//		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"PointLight");
//		break;
//	case LIGHT_TYPE::SPOT_LIGHT:
//		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");
//		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"PointLight");
//		break;
//	}
//}

Entity LightFactory::CreateLight(World* world, LIGHT_TYPE type , LightComponent& l)
{
	Entity entity = world->CreateEntity();

	RenderComponent r{};
	CameraComponent c{};
	TransformComponent t{};


	switch (type)
	{
	case LIGHT_TYPE::DIRECTIONAL_LIGHT:
		r.mMesh		= RESOURCEMANAGER.Get<Mesh>(L"Rectangle");
		r.mMaterials.emplace_back(RESOURCEMANAGER.Get<Material>(L"DirLight"));

		break;
	case LIGHT_TYPE::POINT_LIGHT:
		r.mMesh		= RESOURCEMANAGER.Get<Mesh>(L"Sphere");
		r.mMaterials.emplace_back(RESOURCEMANAGER.Get<Material>(L"PointLight"));
		break;
	case LIGHT_TYPE::SPOT_LIGHT:
		r.mMesh		= RESOURCEMANAGER.Get<Mesh>(L"Sphere");
		r.mMaterials.emplace_back(RESOURCEMANAGER.Get<Material>(L"PointLight"));
		break;
	}

	c.SetScale(1.f);
	c.SetFar(1000.f);
	c.SetWidth(4096);
	c.SetHeight(4096);



	t.mLocalPosition = { Vec3(l.mLightInfo.Position) };
	t.LookAt(Vec3(l.mLightInfo.Direction));

	world->AddComponent<LightComponent>(entity, l);
	world->AddComponent<TransformComponent>(entity,t );
	world->AddComponent<RenderComponent>(entity, r);
	world->AddComponent<CameraComponent>(entity, c);

	return entity;
}
