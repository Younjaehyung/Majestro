#include "pch.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "LightComponent.h"


void LightComponent::SetLightDirection(Vec3 direction)
{
	direction.Normalize();

	mLightInfo.Direction = direction;

	//GetTransform()->LookAt(direction);
}

void LightComponent::SetLightType(LIGHT_TYPE type)
{
	mLightInfo.LightType = static_cast<int32>(type);

	switch (type)
	{
	case LIGHT_TYPE::DIRECTIONAL_LIGHT:
		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Rectangle");
		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"DirLight");

		break;
	case LIGHT_TYPE::POINT_LIGHT:
		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");
		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"PointLight");
		break;
	case LIGHT_TYPE::SPOT_LIGHT:
		mVolumeMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");
		mLightMaterial = RESOURCEMANAGER.Get<Material>(L"PointLight");
		break;
	}
}
