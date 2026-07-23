#include "pch.h"
#include "RenderComponent.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Material.h"
#include "TransformComponent.h"
#include <cstring>


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
	mWorldObbInitialized = false;

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
	mWorldObbInitialized = false;
}

bool RenderComponent::UpdateWorldOBB(TransformComponent* transComp)
{
	if (transComp == nullptr)
		return false;

	const Matrix& world = transComp->GetWorldMatrix();
	if (mWorldObbInitialized &&
		std::memcmp(&mCachedObbWorldMatrix, &world, sizeof(Matrix)) == 0)
	{
		return false;
	}

	XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
	mLocalOBB.GetCorners(corners);

	XMVECTOR vMin = XMVectorReplicate(FLT_MAX);
	XMVECTOR vMax = XMVectorReplicate(-FLT_MAX);
	for (XMFLOAT3& c : corners)
	{
		XMVECTOR p = XMVector3Transform(XMLoadFloat3(&c), world);
		vMin = XMVectorMin(vMin, p);
		vMax = XMVectorMax(vMax, p);
	}

	XMVECTOR center  = (vMin + vMax) * 0.5f;
	XMVECTOR extents = (vMax - vMin) * 0.5f;
	XMStoreFloat3(&mWorldOBB.Center, center);
	XMStoreFloat3(&mWorldOBB.Extents, extents);
	mWorldOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);

	mCachedObbWorldMatrix = world;
	mWorldObbInitialized = true;
	return true;
}

StaticCasterChangeType RenderComponent::UpdateStaticCasterState(bool isStatic)
{
	const Mesh* currentMesh = mMesh.get();
	StaticCasterChangeType changeType = StaticCasterChangeType::None;

	if (mStaticCasterStateInitialized)
	{
		const bool wasStaticCaster =
			mCachedStaticCasterFlag && mCachedStaticCasterMesh != nullptr;
		const bool isStaticCaster = isStatic && currentMesh != nullptr;

		// 동적 오브젝트의 visibility와 mesh 변경은 고정 그림자 캐시에 영향을 주지 않는다.
		if (wasStaticCaster || isStaticCaster)
		{
			const bool membershipChanged = wasStaticCaster != isStaticCaster;
			const bool meshChanged = mCachedStaticCasterMesh != currentMesh;
			const bool visibilityChanged =
				mCachedStaticCasterVisibility != mVisibility;

			if (membershipChanged || meshChanged)
				changeType = StaticCasterChangeType::Bounds;
			else if (visibilityChanged)
				changeType = StaticCasterChangeType::Content;
		}
	}

	mCachedStaticCasterMesh = currentMesh;
	mCachedStaticCasterVisibility = mVisibility;
	mCachedStaticCasterFlag = isStatic;
	mStaticCasterStateInitialized = true;
	return changeType;
}
