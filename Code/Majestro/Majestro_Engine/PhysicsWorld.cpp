#include "pch.h"
#include "EnginePch.h"
#include "PhysicsWorld.h"
#include "Mesh.h"
#include "JoltUtils.h"

PhysicsWorld::PhysicsWorld() = default;
PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::ClearStatic()
{
	mStatics.clear();
	if (mJoltTerrain)
		mJoltTerrain->Clear();
}

SweepHit PhysicsWorld::SphereSweepVsOBB(const Vector3& start, const Vector3& end, float radius)
{
	SweepHit best;
	for (auto& collider : mStatics)
	{
		if (!collider.layerMask) continue;

		SweepHit out{};

		BoundingOrientedBox expanded = collider.obb;
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

		if (out.hit && (!best.hit || out.distance < best.distance))
		{
			out.colliderId = collider.id;
			best = out;
		}
	}
	return best;
}

bool PhysicsWorld::AddStaticCollisionMesh(Entity owner, const CollisionMesh& mesh, const Matrix& worldMatrix)
{
	if (!mJoltTerrain)
		mJoltTerrain = std::make_unique<JoltTerrainState>();
	return mJoltTerrain->AddStaticCollisionMesh(owner, mesh, worldMatrix);
}

bool PhysicsWorld::CastMovingSphereAgainstStatic(const Vector3& start, const Vector3& end, float radius, JoltStaticHit& outHit) const
{
	if (!mJoltTerrain)
	{
		outHit = JoltStaticHit{};
		return false;
	}
	return mJoltTerrain->CastMovingSphere(start, end, radius, outHit);
}

void PhysicsWorld::OptimizeJoltStaticCollision()
{
	if (mJoltTerrain)
		mJoltTerrain->Optimize();
}

bool PhysicsWorld::HasJoltStaticCollision() const
{
	return mJoltTerrain != nullptr;
}
