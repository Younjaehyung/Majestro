#include "pch.h"
#include "PhysicsWorld.h"

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

		if (best.hit && best.distance < best.distance) {
			best.colliderId = collider.id;
			best = out;
		}
			
	}
	return best;
}
