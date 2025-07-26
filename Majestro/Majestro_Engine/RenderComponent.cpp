#include "pch.h"
#include "RenderComponent.h"
#include "Mesh.h"
#include "Material.h"


uint64 RenderComponent::GetInstanceID()
{
	if (mMesh == nullptr || mMaterials.empty())
		return 0;

	//uint64 id = (_mesh->GetID() << 32) | _material->GetID();
	InstanceID instanceID{ mMesh->GetID(), mMaterials[0]->GetID() };
	return instanceID.id;
}