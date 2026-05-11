#include "pch.h"
#include "RenderComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Material.h"
#include "TransformComponent.h"


RenderComponent::RenderComponent()
{
	SetLocalOBB(Vec3(0.f, 0.f, 0.f), Vec3(0.5f, 0.5f, 0.5f));
}

RenderComponent::RenderComponent(shared_ptr<Mesh> mesh, vector<shared_ptr<Material>>& materials) : mMesh(mesh), mMaterials(materials)
{
	SetMesh(mesh);
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

void RenderComponent::SetMesh(shared_ptr<Mesh> mesh)
{
	mMesh = mesh;

	if (mMesh == nullptr)
	{
		return;
	}

	mLocalOBB = mMesh->GetLocalOBB();
	mObbCenter = Vec3(mLocalOBB.Center.x, mLocalOBB.Center.y, mLocalOBB.Center.z);
	mObbHalfExtents = Vec3(mLocalOBB.Extents.x, mLocalOBB.Extents.y, mLocalOBB.Extents.z);
}

void RenderComponent::SetLocalOBB(const Vec3& center, const Vec3& halfExtents)
{
	mObbCenter = center;
	mObbHalfExtents = halfExtents;

	// Manual OBB overrides must update the actual culling OBB, not only the cached editor values.
	mLocalOBB.Center = XMFLOAT3(center.x, center.y, center.z);
	mLocalOBB.Extents = XMFLOAT3(halfExtents.x, halfExtents.y, halfExtents.z);
	mLocalOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
}

void RenderComponent::UpdateWorldOBB(TransformComponent* transComp)
{
	mLocalOBB.Transform(mWorldOBB, transComp->GetWorldMatrix());
}
