#include "pch.h"
#include "PhysicsWorld.h"
#include "World.h"
#include "TransformComponent.h"
#include "ColliderComponent.h"
#include "TagComponent.h"
#include "Mesh.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cmath>

namespace
{
	namespace JoltLayers
	{
		static constexpr JPH::ObjectLayer Terrain = 0;
		static constexpr JPH::BroadPhaseLayer TerrainBroadPhase(0);
	}

	void EnsureJoltInitialized()
	{
		static bool initialized = false;
		if (initialized)
			return;

		// Jolt terrain raycast: initialize Jolt once because mesh shape creation depends on the global factory.
		JPH::RegisterDefaultAllocator();
		if (JPH::Factory::sInstance == nullptr)
			JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
		initialized = true;
	}

	class TerrainBroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual JPH::uint GetNumBroadPhaseLayers() const override
		{
			return 1;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
		{
			(void)inLayer;
			return JoltLayers::TerrainBroadPhase;
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			(void)inLayer;
			return "Terrain";
		}
#endif
	};

	class TerrainObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer, JPH::BroadPhaseLayer inBroadPhaseLayer) const override
		{
			return inLayer == JoltLayers::Terrain && inBroadPhaseLayer == JoltLayers::TerrainBroadPhase;
		}
	};

	class TerrainObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override
		{
			return inLayer1 == JoltLayers::Terrain && inLayer2 == JoltLayers::Terrain;
		}
	};

	JPH::Float3 ToJoltFloat3(const Vec3& v)
	{
		return JPH::Float3(v.x, v.y, v.z);
	}

	float GetVector3Length(DirectX::FXMVECTOR v)
	{
		return DirectX::XMVectorGetX(DirectX::XMVector3Length(v));
	}

	bool IsValidFloat(float value)
	{
		return std::isfinite(value);
	}

	DirectX::XMVECTOR NormalizeAxisOrFallback(DirectX::FXMVECTOR axis, DirectX::FXMVECTOR fallback)
	{
		const float length = GetVector3Length(axis);
		if (!IsValidFloat(length) || length <= 1e-6f)
			return fallback;

		return DirectX::XMVectorScale(axis, 1.0f / length);
	}

	DirectX::XMVECTOR NormalizeQuaternionOrIdentity(DirectX::FXMVECTOR quat)
	{
		const float length = DirectX::XMVectorGetX(DirectX::XMQuaternionLength(quat));
		if (!IsValidFloat(length) || length <= 1e-6f)
			return DirectX::XMQuaternionIdentity();

		return DirectX::XMQuaternionNormalize(quat);
	}

	DirectX::XMVECTOR BuildRotationQuaternionFromWorldMatrix(const Matrix& worldMatrix)
	{
		const DirectX::XMMATRIX matrix = worldMatrix;

		DirectX::XMVECTOR right = NormalizeAxisOrFallback(
			matrix.r[0],
			DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));

		DirectX::XMVECTOR upSource = matrix.r[1];
		upSource = DirectX::XMVectorSubtract(
			upSource,
			DirectX::XMVectorScale(right, DirectX::XMVectorGetX(DirectX::XMVector3Dot(upSource, right))));
		DirectX::XMVECTOR up = NormalizeAxisOrFallback(
			upSource,
			DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		DirectX::XMVECTOR forward = DirectX::XMVector3Cross(right, up);
		forward = NormalizeAxisOrFallback(
			forward,
			DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));

		// 수정 내용
		// OBB Orientation 은 반드시 단위 쿼터니언이어야 한다.
		// 월드 행렬에 반사나 음수 스케일이 섞여도 축 부호만 다른 박스는 같은 형상이므로
		// determinant 가 양수인 정규 직교 회전 행렬을 다시 구성해서 쿼터니언을 만든다.
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixIdentity();
		rotationMatrix.r[0] = DirectX::XMVectorSetW(right, 0.0f);
		rotationMatrix.r[1] = DirectX::XMVectorSetW(up, 0.0f);
		rotationMatrix.r[2] = DirectX::XMVectorSetW(forward, 0.0f);

		return NormalizeQuaternionOrIdentity(DirectX::XMQuaternionRotationMatrix(rotationMatrix));
	}

	void TransformOBBSafely(const BoundingOrientedBox& localOBB, BoundingOrientedBox& worldOBB, const Matrix& worldMatrix)
	{
		const DirectX::XMMATRIX matrix = worldMatrix;

		const DirectX::XMVECTOR localCenter = DirectX::XMLoadFloat3(&localOBB.Center);
		const DirectX::XMVECTOR worldCenter = DirectX::XMVector3Transform(localCenter, matrix);

		const DirectX::XMVECTOR localExtents = DirectX::XMLoadFloat3(&localOBB.Extents);
		DirectX::XMFLOAT3 scale{};
		scale.x = GetVector3Length(matrix.r[0]);
		scale.y = GetVector3Length(matrix.r[1]);
		scale.z = GetVector3Length(matrix.r[2]);
		if (!IsValidFloat(scale.x)) scale.x = 0.0f;
		if (!IsValidFloat(scale.y)) scale.y = 0.0f;
		if (!IsValidFloat(scale.z)) scale.z = 0.0f;
		const DirectX::XMVECTOR worldExtents = DirectX::XMVectorMultiply(
			localExtents,
			DirectX::XMLoadFloat3(&scale));

		const DirectX::XMVECTOR localRotation = NormalizeQuaternionOrIdentity(DirectX::XMLoadFloat4(&localOBB.Orientation));
		const DirectX::XMVECTOR worldRotation = BuildRotationQuaternionFromWorldMatrix(worldMatrix);
		const DirectX::XMVECTOR finalRotation = NormalizeQuaternionOrIdentity(
			DirectX::XMQuaternionMultiply(localRotation, worldRotation));

		DirectX::XMStoreFloat3(&worldOBB.Center, worldCenter);
		DirectX::XMStoreFloat3(&worldOBB.Extents, worldExtents);
		DirectX::XMStoreFloat4(&worldOBB.Orientation, finalRotation);
	}
}

struct JoltTerrainState
{
	JoltTerrainState()
	{
		EnsureJoltInitialized();

		// Jolt terrain raycast: this PhysicsSystem is query only, so one terrain layer is enough.
		PhysicsSystem.Init(
			4096,
			0,
			4096,
			4096,
			BroadPhaseLayerInterface,
			ObjectVsBroadPhaseLayerFilter,
			ObjectLayerPairFilter);
	}

	~JoltTerrainState()
	{
		Clear();
	}

	void Clear()
	{
		JPH::BodyInterface& bodyInterface = PhysicsSystem.GetBodyInterface();
		for (const JPH::BodyID& bodyID : BodyIDs)
		{
			if (bodyID.IsInvalid())
				continue;
			if (bodyInterface.IsAdded(bodyID))
				bodyInterface.RemoveBody(bodyID);
			bodyInterface.DestroyBody(bodyID);
		}
		BodyIDs.clear();
	}

	bool AddMesh(const CollisionMesh& mesh, const Matrix& worldMatrix)
	{
		const vector<Vertex>& sourceVertices = mesh.GetVertexBuffer();
		const vector<uint32>& sourceIndices = mesh.GetIndexBuffer();
		if (sourceVertices.empty() || sourceIndices.size() < 3)
			return false;

		JPH::VertexList vertices;
		vertices.reserve(sourceVertices.size());
		for (const Vertex& vertex : sourceVertices)
		{
			const Vec3 worldPos = Vec3::Transform(vertex.pos, worldMatrix);
			vertices.push_back(ToJoltFloat3(worldPos));
		}

		JPH::IndexedTriangleList triangles;
		triangles.reserve(sourceIndices.size() / 3);
		for (size_t i = 0; i + 2 < sourceIndices.size(); i += 3)
		{
			const uint32 i0 = sourceIndices[i + 0];
			const uint32 i1 = sourceIndices[i + 1];
			const uint32 i2 = sourceIndices[i + 2];
			if (i0 >= sourceVertices.size() || i1 >= sourceVertices.size() || i2 >= sourceVertices.size())
				continue;
			triangles.push_back(JPH::IndexedTriangle(i0, i1, i2, 0));
		}

		if (triangles.empty())
			return false;

		// Jolt terrain raycast: bake the world transform into vertices so the body can stay at identity.
		JPH::MeshShapeSettings shapeSettings(std::move(vertices), std::move(triangles));
		JPH::Shape::ShapeResult shapeResult = shapeSettings.Create();
		if (!shapeResult.IsValid())
			return false;

		JPH::BodyCreationSettings bodySettings(
			shapeResult.Get(),
			JPH::RVec3::sZero(),
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Static,
			JoltLayers::Terrain);

		JPH::BodyID bodyID = PhysicsSystem.GetBodyInterface().CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);
		if (bodyID.IsInvalid())
			return false;

		BodyIDs.push_back(bodyID);
		return true;
	}

	bool RayCastHeight(const Vec3& position, float& outHeight) const
	{
		if (BodyIDs.empty())
			return false;

		static constexpr float RayStartOffset = 100000.0f;
		static constexpr float RayLength = 200000.0f;

		const JPH::RVec3 origin(position.x, position.y + RayStartOffset, position.z);
		const JPH::Vec3 direction(0.0f, -RayLength, 0.0f);
		const JPH::RRayCast ray(origin, direction);

		JPH::RayCastResult hit;
		if (!PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, hit))
			return false;

		const JPH::RVec3 hitPos = ray.GetPointOnRay(hit.mFraction);
		outHeight = static_cast<float>(hitPos.GetY());
		return true;
	}

	void Optimize()
	{
		if (!BodyIDs.empty())
			PhysicsSystem.OptimizeBroadPhase();
	}

	TerrainBroadPhaseLayerInterface BroadPhaseLayerInterface;
	TerrainObjectVsBroadPhaseLayerFilter ObjectVsBroadPhaseLayerFilter;
	TerrainObjectLayerPairFilter ObjectLayerPairFilter;
	JPH::PhysicsSystem PhysicsSystem;
	std::vector<JPH::BodyID> BodyIDs;
};

PhysicsWorld::PhysicsWorld() = default;

PhysicsWorld::PhysicsWorld(World* world) : mWorld(world)
{
	Initialize();
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::ClearStatic()
{
	staticObjects.clear();
	nodes.clear();
	root = -1;
	if (mJoltTerrain)
		mJoltTerrain->Clear();
}

void PhysicsWorld::Initialize()
{ 
	RebuildStaticBVH();
}

size_t PhysicsWorld::CountStaticCollisionEntities() const
{
	if (!mWorld)
		return 0;
	if (false == mWorld->HasComponentPool<StaticComponent>()) return 0;
	if (false == mWorld->HasComponentPool<TransformComponent>()) return 0;
	if (false == mWorld->HasComponentPool<BoxColliderComponent>()) return 0;

	return mWorld->GetEntitiesWithComponents<StaticComponent, TransformComponent, BoxColliderComponent>().size();
}

bool PhysicsWorld::SyncStaticBVHIfNeeded()
{
	const size_t staticCount = CountStaticCollisionEntities();

	// Fix: StaticComponent can be added after PhysicsWorld construction, so rebuild when the BVH cache is stale.
	if (staticCount != staticObjects.size())
	{
		RebuildStaticBVH();
		return true;
	}

	// Fix: a nonzero static set must never run with an empty or invalid BVH.
	if (staticCount > 0 && (root < 0 || nodes.empty()))
	{
		RebuildStaticBVH();
		return true;
	}

	return false;
}

void PhysicsWorld::RebuildStaticBVH()
{
	ClearStatic();

	if (!mWorld)
		return;
	if (false == mWorld->HasComponentPool<StaticComponent>())return;
	auto staticEntities = mWorld->GetEntitiesWithComponents<StaticComponent, TransformComponent, BoxColliderComponent>();
	staticObjects.reserve(staticEntities.size());

	for (auto e : staticEntities)
	{
		auto* tr = mWorld->GetComponent<TransformComponent>(e);
		auto* col = mWorld->GetComponent<BoxColliderComponent>(e);
		if (!tr || !col)
			continue;
		UpdateWorldOBB(tr, col);
		const AABB2D bounds = BuildAABBFromOBB(col->mWorldOBB);

		staticObjects.push_back(StaticProxy{ e, col, bounds });

		if (col->mRayCastMesh)
		{
			// Jolt terrain raycast: register render mesh triangles alongside the existing OBB proxy.
			AddTerrainRayCastMesh(*col->mRayCastMesh, tr->mWorldMatrix);
		}
	}
	
	if (staticObjects.empty())
		return;

	nodes.reserve(staticObjects.size() * 2);

	root = BuildStaticBVHRecursive(staticObjects, nodes, 0, static_cast<int>(staticObjects.size()));
	if (mJoltTerrain)
		mJoltTerrain->Optimize();

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

		// 수정 내용
		// StaticProxy 에 저장된 ColliderBox 포인터는 ComponentPool vector 재할당 후 무효화될 수 있다.
		// 스윕 쿼리 시 Entity 로 현재 컴포넌트를 다시 찾아서 댕글링 포인터 접근을 막는다.
		BoxColliderComponent* colliderBox = mWorld ? mWorld->GetComponent<BoxColliderComponent>(collider.ColliderEntity) : collider.ColliderBox;
		collider.ColliderBox = colliderBox;
		if (!colliderBox)
			continue;

		BoundingOrientedBox expanded = colliderBox->mWorldOBB;
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
	// Fix: an empty static BVH must be a valid no-hit state, not an index into nodes.
	if (root < 0 || nodes.empty()) return;

	std::vector<int> stack;                 // [수정] 동적 스택으로 안전하게
	stack.reserve(64);
	stack.push_back(root);

	while (!stack.empty())
	{
		const int nodeIndex = stack.back();
		stack.pop_back();
		if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size()))
			continue;

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
	// Fix: callers can rebuild with zero static colliders, so do not read proxies[0].
	if (count <= 0)
		return -1;

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
	float joltHeight = 0.0f;
	if (TryQueryTerrainHeight(position, joltHeight))
		return joltHeight;

	float bestHeight = -FLT_MAX;
	
	for (const auto& collider : staticObjects)
	{
		// 수정 내용
		// StaticProxy 의 ColliderBox 캐시는 컴포넌트 추가로 무효화될 수 있으므로 높이 조회도 Entity 기반으로 접근한다.
		const BoxColliderComponent* colliderBox = mWorld ? mWorld->GetComponent<BoxColliderComponent>(collider.ColliderEntity) : collider.ColliderBox;
		if (!colliderBox)
			continue;

		const BoundingOrientedBox& obb = colliderBox->mWorldOBB;

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

bool PhysicsWorld::TryQueryTerrainHeight(const Vector3& position, float& outHeight) const
{
	// Jolt terrain raycast: returns false when no mesh terrain has been registered, preserving old fallback behavior.
	if (!mJoltTerrain)
		return false;
	return mJoltTerrain->RayCastHeight(position, outHeight);
}

bool PhysicsWorld::AddTerrainRayCastMesh(const CollisionMesh& mesh, const Matrix& worldMatrix)
{
	if (!mJoltTerrain)
		mJoltTerrain = std::make_unique<JoltTerrainState>();
	return mJoltTerrain->AddMesh(mesh, worldMatrix);
}

bool PhysicsWorld::HasJoltTerrain() const
{
	return mJoltTerrain != nullptr;
}


void PhysicsWorld::UpdateWorldOBB(const TransformComponent* tr, BoxColliderComponent* col)
{

	// 수정 내용
	// DirectX BoundingOrientedBox::Transform 은 반사 행렬이나 음수 스케일이 섞이면
	// Intersects 에서 요구하는 단위 쿼터니언 Orientation 을 보장하지 못할 수 있다.
	// 서버 충돌 판정은 항상 단위 쿼터니언을 저장하도록 안전 변환 경로를 사용한다.
	TransformOBBSafely(col->mLocalOBB, col->mWorldOBB, tr->mWorldMatrix);


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
	// 수정 내용
	// 외부에서 OBB 를 넘겨 갱신하는 경로도 UpdateWorldOBB 와 동일하게 단위 쿼터니언을 보장한다.
	TransformOBBSafely(obb, col->mWorldOBB, tr->mWorldMatrix);
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
