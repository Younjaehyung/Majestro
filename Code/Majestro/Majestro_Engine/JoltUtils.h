#pragma once
#include "Mesh.h"

	namespace JoltLayers
	{
		static constexpr JPH::ObjectLayer Terrain = 0;
		static constexpr JPH::ObjectLayer StaticCollision = 1;
		static constexpr JPH::BroadPhaseLayer TerrainBroadPhase(0);
	}

	static void JoltTraceImpl(const char* inFMT, ...)
	{
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);
		OutputDebugStringA("[Jolt] ");
		OutputDebugStringA(buffer);
		OutputDebugStringA("\n");
	}

#ifdef JPH_ENABLE_ASSERTS
	static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine)
	{
		OutputDebugStringA(inFile);
		char lineBuf[32];
		snprintf(lineBuf, sizeof(lineBuf), ":%u: ", inLine);
		OutputDebugStringA(lineBuf);
		OutputDebugStringA(inExpression);
		if (inMessage)
		{
			OutputDebugStringA(" - ");
			OutputDebugStringA(inMessage);
		}
		OutputDebugStringA("\n");
		return true;
	}
#endif

	void EnsureJoltInitialized()
	{
		static bool initialized = false;
		if (initialized)
			return;

		JPH::RegisterDefaultAllocator();
		JPH::Trace = JoltTraceImpl;
		JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailedImpl;)
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
			const bool validLayer1 = inLayer1 == JoltLayers::Terrain || inLayer1 == JoltLayers::StaticCollision;
			const bool validLayer2 = inLayer2 == JoltLayers::Terrain || inLayer2 == JoltLayers::StaticCollision;
			return validLayer1 && validLayer2;
		}
	};

	JPH::Float3 ToJoltFloat3(const Vec3& v)
	{
		return JPH::Float3(v.x, v.y, v.z);
	}

	JPH::Vec3 ToJoltVec3(const Vec3& v)
	{
		return JPH::Vec3(v.x, v.y, v.z);
	}

	JPH::RVec3 ToJoltRVec3(const Vec3& v)
	{
		return JPH::RVec3(v.x, v.y, v.z);
	}

	Vec3 FromJoltVec3(const JPH::Vec3& v)
	{
		return Vec3(v.GetX(), v.GetY(), v.GetZ());
	}

	Entity EntityFromJoltUserData(JPH::uint64 userData)
	{
		if (userData == 0)
			return Entity{};
		return Entity(static_cast<EntityID>(userData));
	}


struct JoltTerrainState
{
	JoltTerrainState()
	{
		EnsureJoltInitialized();

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

	bool AddMeshBody(Entity owner, const CollisionMesh& mesh, const Matrix& worldMatrix, JPH::ObjectLayer objectLayer)
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

		JPH::MeshShapeSettings shapeSettings(std::move(vertices), std::move(triangles));
		JPH::Shape::ShapeResult shapeResult = shapeSettings.Create();
		if (!shapeResult.IsValid())
			return false;

		JPH::BodyCreationSettings bodySettings(
			shapeResult.Get(),
			JPH::RVec3::sZero(),
			JPH::Quat::sIdentity(),
			JPH::EMotionType::Static,
			objectLayer);
		bodySettings.mUserData = owner.IsValid() ? static_cast<JPH::uint64>(owner.GetID()) : 0;

		JPH::BodyID bodyID = PhysicsSystem.GetBodyInterface().CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);
		if (bodyID.IsInvalid())
			return false;

		BodyIDs.push_back(bodyID);
		return true;
	}

	bool AddStaticCollisionMesh(Entity owner, const CollisionMesh& mesh, const Matrix& worldMatrix)
	{
		return AddMeshBody(owner, mesh, worldMatrix, JoltLayers::StaticCollision);
	}

	bool CastMovingSphere(const Vec3& start, const Vec3& end, float radius, JoltStaticHit& outHit) const
	{
		outHit = JoltStaticHit{};
		if (BodyIDs.empty() || radius <= 0.0f)
			return false;

		const Vec3 delta = end - start;
		const float length = delta.Length();
		if (length <= 1e-6f)
			return false;

		JPH::SphereShape sphere(radius);
		const JPH::RMat44 startTransform = JPH::RMat44::sTranslation(ToJoltRVec3(start));
		const JPH::RShapeCast shapeCast(&sphere, JPH::Vec3::sReplicate(1.0f), startTransform, ToJoltVec3(delta));

		JPH::ShapeCastSettings settings;
		settings.SetBackFaceMode(JPH::EBackFaceMode::CollideWithBackFaces);
		settings.mReturnDeepestPoint = true;

		JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
		JPH::SpecifiedObjectLayerFilter staticOnly(JoltLayers::StaticCollision);
		PhysicsSystem.GetNarrowPhaseQuery().CastShape(
			shapeCast,
			settings,
			JPH::RVec3::sZero(),
			collector,
			{},
			staticOnly);

		if (!collector.HadHit())
			return false;

		const JPH::ShapeCastResult& result = collector.mHit;
		const float fraction = (std::max)(0.0f, (std::min)(1.0f, result.mFraction));
		outHit.hit = true;
		outHit.fraction = fraction;
		outHit.distance = length * fraction;
		outHit.point = FromJoltVec3(result.mContactPointOn2);

		JPH::BodyLockRead bodyLock(PhysicsSystem.GetBodyLockInterface(), result.mBodyID2);
		if (bodyLock.Succeeded())
		{
			const JPH::Body& body = bodyLock.GetBody();
			outHit.colliderId = EntityFromJoltUserData(body.GetUserData());
			const JPH::Vec3 normal = body.GetWorldSpaceSurfaceNormal(
				result.mSubShapeID2,
				JPH::RVec3(
					result.mContactPointOn2.GetX(),
					result.mContactPointOn2.GetY(),
					result.mContactPointOn2.GetZ()));
			outHit.normal = FromJoltVec3(normal);
		}

		if (outHit.point.LengthSquared() <= 1e-8f)
			outHit.point = start + delta * fraction;
		return true;
	}

	void Optimize()
	{
		if (!BodyIDs.empty())
			PhysicsSystem.OptimizeBroadPhase();
	}

	TerrainBroadPhaseLayerInterface      BroadPhaseLayerInterface;
	TerrainObjectVsBroadPhaseLayerFilter ObjectVsBroadPhaseLayerFilter;
	TerrainObjectLayerPairFilter         ObjectLayerPairFilter;
	JPH::PhysicsSystem                   PhysicsSystem;
	std::vector<JPH::BodyID>             BodyIDs;
};
