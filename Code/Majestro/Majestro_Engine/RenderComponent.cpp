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

void RenderComponent::UpdateWorldOBB(const TransformComponent* transformComponent)
{
	if (!transformComponent)
		return;

	Vec scale;
	Vec rotation;
	Vec translation;
	const Matrix worldMatrix = transformComponent->mWorldMatrix;

	if (!XMMatrixDecompose(&scale, &rotation, &translation, worldMatrix))
		return;

	Vec3 scaleF;
	Vec3 translationF;
	Vec4 rotationF;
	XMStoreFloat3(&scaleF, scale);
	XMStoreFloat3(&translationF, translation);
	XMStoreFloat4(&rotationF, XMQuaternionNormalize(rotation));

	const Vec3 worldPos = Vec3(translationF.x, translationF.y, translationF.z);
	const Vec3 absScale = Vec3(fabsf(scaleF.x), fabsf(scaleF.y), fabsf(scaleF.z));

	const Vec3 scaledCenter = Vec3(
		mObbCenter.x * absScale.x,
		mObbCenter.y * absScale.y,
		mObbCenter.z * absScale.z);

	const Vec localCenter = XMVectorSet(scaledCenter.x, scaledCenter.y, scaledCenter.z, 0.0f);
	const Vec rotatedCenter = XMVector3Rotate(localCenter, XMLoadFloat4(&rotationF));
	XMFLOAT3 rotatedCenterF;
	XMStoreFloat3(&rotatedCenterF, rotatedCenter);

	const Vec3 worldCenter = worldPos + rotatedCenterF;
	const Vec3 worldExtents = Vec3(
		mObbHalfExtents.x * absScale.x,
		mObbHalfExtents.y * absScale.y,
		mObbHalfExtents.z * absScale.z);

	mWorldOBB.Center = worldCenter;
	mWorldOBB.Extents = worldExtents;
	mWorldOBB.Orientation = rotationF;
}