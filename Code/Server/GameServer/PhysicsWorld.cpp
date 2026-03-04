#include "pch.h"
#include "PhysicsWorld.h"
#include "World.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "TagComponent.h"

void PhysicsWorld::Initialize()
{ 
	if (false == mWorld->HasComponentPool<StaticComponent>())return;
	auto staticEntities = mWorld->GetEntitiesWithComponents<StaticComponent, TransformComponent, BoxColliderComponent>();

	for (auto e : staticEntities)
	{
		auto* tr = mWorld->GetComponent<TransformComponent>(e);
		auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
		if (!tr || !col)
			continue;
		UpdateWorldOBB(tr, col);
		const AABB2D bounds = BuildAABBFromOBB(col->mWorldOBB);

		staticObjects.push_back(StaticProxy{ e, col, bounds });

	}
	

	nodes.reserve(staticObjects.size() * 2);

	root = BuildStaticBVHRecursive(staticObjects, nodes, 0, static_cast<int>(staticObjects.size()));

}

AABB2D PhysicsWorld::MergeAABB(const AABB2D& a, const AABB2D& b)
{
	return AABB2D{
		(std::min)(a.minX, b.minX),
		(std::max)(a.maxX, b.maxX),
		(std::min)(a.minZ, b.minZ),
		(std::max)(a.maxZ, b.maxZ)
	};
}

bool PhysicsWorld::OverlapAABB(const AABB2D& a, const AABB2D& b)
{
	if (a.maxX < b.minX || b.maxX < a.minX)
		return false;
	if (a.maxZ < b.minZ || b.maxZ < a.minZ)
		return false;
	return true;
}


SweepHit PhysicsWorld::SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius)
{
	SweepHit best;
	for (auto& collider : staticObjects)
	{
		
		//if (!collider.layerMask) continue;

		SweepHit out{};


		BoundingOrientedBox expanded = collider.ColliderBox->mWorldOBB;
		expanded.Extents.x += radius;
		expanded.Extents.y += radius;
		expanded.Extents.z += radius;

		Vec3 s = start;
		Vec3 e = end;
		Vec3 dir = e - s;

		float segLen = XMVectorGetX(XMVector3Length(dir));
		if (segLen <= 1e-6f)
			return out;

		dir.Normalize();

		// 시작 오버랩 처리
		if (expanded.Contains(s) != ContainmentType::DISJOINT)
		{
			out.hit = true;
			out.distance = 0.0f;
			return out;
		}

		float dist = 0.0f;
		
		if (expanded.Intersects(s, dir, dist))
		{
			if (dist >= 0.0f && dist <= segLen)
			{
				out.hit = true;
				out.distance = dist;
			}
		}

		if (best.hit || out.distance < best.distance) {
			out.colliderId = collider.ColliderEntity;
			best = out;
		}
			
	}
	return best;
}


void PhysicsWorld::QueryStaticBVH(const AABB2D& query, std::vector<int>& outIndices)
{
	if (root < 0) return;

	std::vector<int> stack;                 // [수정] 동적 스택으로 안전하게
	stack.reserve(64);
	stack.push_back(root);

	while (!stack.empty())
	{
		const int nodeIndex = stack.back();
		stack.pop_back();

		const BVHNode& node = nodes[nodeIndex];

		if (!OverlapAABB(node.bounds, query))
			continue;

		if (node.IsLeaf())
		{
			for (int i = 0; i < node.count; ++i)
				outIndices.push_back(node.start + i);
			continue;
		}

		if (node.left >= 0)  stack.push_back(node.left);
		if (node.right >= 0) stack.push_back(node.right);
	}
}

int PhysicsWorld::BuildStaticBVHRecursive(
	std::vector<StaticProxy>& proxies,
	std::vector<BVHNode>& nodes,
	int start,
	int count)
{
	const int nodeIndex = static_cast<int>(nodes.size());
	nodes.push_back(BVHNode{});

	BVHNode& node = nodes[nodeIndex];
	node.start = start;
	node.count = count;
	node.bounds = proxies[start].bounds;

	for (int i = 1; i < count; ++i)
	{
		node.bounds = MergeAABB(node.bounds, proxies[start + i].bounds);
	}

	constexpr int kLeafSize = 4;
	if (count <= kLeafSize)
		return nodeIndex;

	const float extentX = node.bounds.maxX - node.bounds.minX;
	const float extentZ = node.bounds.maxZ - node.bounds.minZ;
	const bool splitX = extentX >= extentZ;

	const int mid = start + count / 2;
	std::nth_element(
		proxies.begin() + start,
		proxies.begin() + mid,
		proxies.begin() + start + count,
		[splitX](const StaticProxy& lhs, const StaticProxy& rhs)
		{
			const float lhsCenter = splitX
				? (lhs.bounds.minX + lhs.bounds.maxX) * 0.5f
				: (lhs.bounds.minZ + lhs.bounds.maxZ) * 0.5f;
			const float rhsCenter = splitX
				? (rhs.bounds.minX + rhs.bounds.maxX) * 0.5f
				: (rhs.bounds.minZ + rhs.bounds.maxZ) * 0.5f;
			return lhsCenter < rhsCenter;
		});

	node.left = BuildStaticBVHRecursive(proxies, nodes, start, mid - start);
	node.right = BuildStaticBVHRecursive(proxies, nodes, mid, start + count - mid);
	node.count = 0;
	return nodeIndex;
}

float PhysicsWorld::QueryHeightAtPosition(const Vector3& position)	// To - Do : 충돌 후보 최적화
{
	float bestHeight = -FLT_MAX;
	
	for (const auto& collider : staticObjects)
	{
		const BoundingOrientedBox& obb = collider.ColliderBox->mWorldOBB;

		const Vector3 obbCenter = obb.Center;
		const Vector3 obbUp = Vector3(obb.Orientation.x, obb.Orientation.y, obb.Orientation.z); // OBB의 Up 벡터
		const float distance = (position - obbCenter).Dot(obbUp);

		if (distance >= 0)
		{
			const float height = obbCenter.y + distance; 
			if (height > bestHeight)
				bestHeight = height;
		}
	}
	return bestHeight;
}


void PhysicsWorld::UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col)
{

	col->mLocalOBB.BoundingOrientedBox::Transform(col->mWorldOBB, tr->mWorldMatrix);


 //   XMVECTOR S, R, T;

 //   // [수정] SimpleMath::Matrix -> XMMATRIX 변환
 //   const XMMATRIX M = tr->mWorldMatrix; // SimpleMath::Matrix는 XMMATRIX로 암시 변환되는 경우가 많음

 //   if (!XMMatrixDecompose(&S, &R, &T, M))
 //       return;

 //   // scale / rotation(quat) / translation 추출
 //   const XMFLOAT3 s3 = {};
 //   const XMFLOAT4 r4 = {};
 //   const XMFLOAT3 t3 = {};
 //   XMFLOAT3 sF, tF;
 //   XMFLOAT4 rF;
 //   XMStoreFloat3(&sF, S);
 //   XMStoreFloat3(&tF, T);
 //   XMStoreFloat4(&rF, XMQuaternionNormalize(R));

 //   const Vec3 worldPos = Vec3(tF.x, tF.y, tF.z);

 //   // 로컬 Center 오프셋을 월드 회전으로 회전
 //   const XMVECTOR localCenter = XMVectorSet(col->mCenter.x, col->mCenter.y, col->mCenter.z, 0.0f);
 //   const XMVECTOR rotatedOffV = XMVector3Rotate(localCenter, XMLoadFloat4(&rF));
 //   XMFLOAT3 rotatedOffF;
 //   XMStoreFloat3(&rotatedOffF, rotatedOffV);

 //   const Vec3 worldCenter = worldPos + Vec3(rotatedOffF.x, rotatedOffF.y, rotatedOffF.z);

 //   // Extents
 //   Vec3 ext = col->mHalfExtents;


 //   col->mWorldOBB.Center = XMFLOAT3(worldCenter.x, worldCenter.y, worldCenter.z);
 //   col->mWorldOBB.Extents = XMFLOAT3(ext.x, ext.y, ext.z);
 //   col->mWorldOBB.Orientation = XMFLOAT4(rF.x, rF.y, rF.z, rF.w);

	//
	//{
	//	// 로컬 OBB 구성 (Center 오프셋 + HalfExtents, 회전 없음)
	//	BoundingOrientedBox localOBB;
	//	localOBB.Center = XMFLOAT3(col->mCenter.x, col->mCenter.y, col->mCenter.z);
	//	localOBB.Extents = XMFLOAT3(col->mHalfExtents.x, col->mHalfExtents.y, col->mHalfExtents.z);
	//	localOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f); // identity quaternion

	//	// 월드 매트릭스로 한 번에 변환
	//	localOBB.Transform(col->mWorldOBB, tr->mWorldMatrix);
	//}
}

void PhysicsWorld::SetWorldOBB(BoundingOrientedBox obb, const TransformComponent* tr, BoxColliderComponent* col)
{
	obb.BoundingOrientedBox::Transform(col->mWorldOBB, tr->mWorldMatrix);
}


AABB2D PhysicsWorld::BuildAABBFromOBB(const BoundingOrientedBox& obb)
{
    XMFLOAT3 corners[8];
    obb.GetCorners(corners);

    AABB2D bounds{ corners[0].x, corners[0].x, corners[0].z, corners[0].z };

    for (const auto& c : corners)
    {
        bounds.minX = (std::min)(bounds.minX, c.x);
        bounds.maxX = (std::max)(bounds.maxX, c.x);
        bounds.minZ = (std::min)(bounds.minZ, c.z);
        bounds.maxZ = (std::max)(bounds.maxZ, c.z);
    }

    return bounds;
}
