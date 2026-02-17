#include "pch.h"
#include "RenderComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Material.h"
#include "TransformComponent.h"


RenderComponent::RenderComponent()
{
}

RenderComponent::RenderComponent(shared_ptr<Mesh> mesh, vector<shared_ptr<Material>>& materials) : mMesh(mesh), mMaterials(materials)
{
	mMesh = mesh;
	mMaterials = materials;
}

uint64 RenderComponent::GetInstanceID()
{
	if (mMesh == nullptr || mMaterials.empty())
		return 0;

	//uint64 id = (_mesh->GetID() << 32) | _material->GetID();
	InstanceID instanceID{ mMesh->GetID(), mMaterials[0]->GetID() };
	return instanceID.ID;
}

void RenderComponent::SetLocalOBB(const Vec3& center, const Vec3& halfExtents)
{
	mObbCenter = center;
	mObbHalfExtents = halfExtents;
}

void RenderComponent::UpdateWorldOBB(TransformComponent* transComp)
{
	mLocalOBB.Transform(mWorldOBB, transComp->GetWorldMatrix());
}