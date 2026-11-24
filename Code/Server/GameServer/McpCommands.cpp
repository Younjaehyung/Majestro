#include "pch.h"
#include "McpCommands.h"
#include "McpState.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "EnemyComponent.h"
#include "PlayerComponent.h"
#include "NetEntityComponent.h"
#include "GameEvents.h"
#include "GamePhase.h"
#include "EnemySystem.h"
#include "DamageSystem.h"
#include "NavMeshLoader.h"

// Unity 빌드라 익명 namespace 가 다른 .cpp 와 합쳐진다. 이름이 겹치지 않게 따로 둔다.
namespace McpCommandsDetail
{
	World* FindWorld(const json& request, json& error)
	{
		uint32 roomId = 0;
		Scene* scene = McpFindRoom(request, roomId, error);
		return scene ? scene->GetWorld().get() : nullptr;
	}

	Entity FirstPlayer(World* world)
	{
		if (!world->HasComponentPool<MainPlayerComponent>())
			return Entity();
		auto players = world->GetEntitiesWithComponent<MainPlayerComponent>();
		return players.empty() ? Entity() : players[0];
	}

	Entity FindByNetId(World* world, uint64 netId)
	{
		if (!world->HasComponentPool<NetEntityComponent>())
			return Entity();
		for (Entity e : world->GetEntitiesWithComponent<NetEntityComponent>())
			if (world->GetComponent<NetEntityComponent>(e)->mNetEntityId == netId)
				return e;
		return Entity();
	}

	// netId 가 있으면 그 엔티티, 없으면 첫 플레이어
	Entity Target(World* world, const json& request)
	{
		return request.contains("netId") ? FindByNetId(world, request["netId"].get<uint64>()) : FirstPlayer(world);
	}

	Vec3 ReadVec(const json& v)
	{
		return Vec3(v.at(0).get<float>(), v.at(1).get<float>(), v.at(2).get<float>());
	}

	json VecJson(const Vec3& v)
	{
		return json::array({ v.x, v.y, v.z });
	}

	// 낙사 처리(DeathSystem::BeginFallDeath)와 같은 치명 피해. Slime 은 치명타만 받으므로 치명타로 보낸다.
	bool Kill(World* world, Entity enemy)
	{
		const HealthComponent* health = world->GetComponent<HealthComponent>(enemy);
		auto eventManager = world->GetEventManager();
		if (!health || health->IsDead() || !eventManager)
			return false;
		int32 lethal = health->mMaxHp + 1;
		if (const ArmorComponent* armor = world->HasComponentPool<ArmorComponent>() ? world->GetComponent<ArmorComponent>(enemy) : nullptr)
			lethal += armor->mMaxArmor;
		EvDamage damage{};
		damage.target = enemy;
		damage.amount = lethal;
		damage.instigator = Entity{};
		damage.isCritical = true;
		eventManager->Enqueue<EvDamage>(damage);
		return true;
	}
}

json McpTeleport(const json& request)
{
	using namespace McpCommandsDetail;
	json error;
	World* world = FindWorld(request, error);
	if (!world)
		return error;
	const Entity player = Target(world, request);
	if (!player.IsValid() || !world->GetComponent<MainPlayerComponent>(player))
		return { { "ok", false }, { "error", "player not found" } };
	const Vec3 placed = PlacePlayerAt(world, player, ReadVec(request.at("pos")));
	return { { "ok", true }, { "entity", player.GetID() }, { "pos", VecJson(placed) } };
}

json McpSetHp(const json& request)
{
	using namespace McpCommandsDetail;
	json error;
	World* world = FindWorld(request, error);
	if (!world)
		return error;
	const Entity target = Target(world, request);
	HealthComponent* health = target.IsValid() && world->HasComponentPool<HealthComponent>()
		? world->GetComponent<HealthComponent>(target) : nullptr;
	if (!health)
		return { { "ok", false }, { "error", "target with health not found" } };

	const int32 hp = (std::clamp)(request.at("hp").get<int32>(), 0, health->mMaxHp);
	const bool isPlayer = world->GetComponent<MainPlayerComponent>(target) != nullptr;
	if (hp == 0 && !isPlayer)
		return { { "ok", Kill(world, target) }, { "result", "killed" } };

	auto eventManager = world->GetEventManager();
	health->mCurrentHp = hp;
	if (eventManager)
	{
		eventManager->Enqueue<EvHealthChanged>({ target, health->mCurrentHp, health->mMaxHp });
		if (hp == 0)	// DamageSystem 의 플레이어 사망과 같은 요청
			eventManager->Enqueue<EvPlayerDeathRequest>(
				{ target, static_cast<int8>(PlayerDeathCause::Health), 10.0f, false });
	}
	return { { "ok", true }, { "entity", target.GetID() }, { "hp", hp }, { "maxHp", health->mMaxHp } };
}

json McpKillEnemies(const json& request)
{
	using namespace McpCommandsDetail;
	json error;
	World* world = FindWorld(request, error);
	if (!world)
		return error;
	if (!world->HasComponentPool<EnemyComponent>())
		return { { "ok", true }, { "killed", 0 } };

	if (request.contains("netId"))
	{
		const Entity enemy = FindByNetId(world, request["netId"].get<uint64>());
		if (!enemy.IsValid() || !world->GetComponent<EnemyComponent>(enemy))
			return { { "ok", false }, { "error", "enemy not found" } };
		return { { "ok", true }, { "killed", Kill(world, enemy) ? 1 : 0 } };
	}

	// radius 가 있으면 첫 플레이어 기준 수평 거리 안만
	const float radius = request.value("radius", 0.f);
	Vec3 center{};
	if (radius > 0.f)
	{
		const Entity player = FirstPlayer(world);
		const TransformComponent* transform = player.IsValid() ? world->GetComponent<TransformComponent>(player) : nullptr;
		if (!transform)
			return { { "ok", false }, { "error", "radius needs a player" } };
		center = transform->mWorldPosition;
	}
	int killed = 0;
	for (Entity e : world->GetEntitiesWithComponents<TransformComponent, EnemyComponent>())
	{
		if (radius > 0.f)
		{
			const Vec3 at = world->GetComponent<TransformComponent>(e)->mWorldPosition;
			if (std::hypot(at.x - center.x, at.z - center.z) > radius)
				continue;
		}
		killed += Kill(world, e) ? 1 : 0;
	}
	return { { "ok", true }, { "killed", killed } };
}

json McpSpawnEnemy(const json& request)
{
	using namespace McpCommandsDetail;
	json error;
	World* world = FindWorld(request, error);
	if (!world)
		return error;

	const int type = request.at("type").get<int>();
	if (type < EnemyType::HornMan || type > EnemyType::Dragon)
		return { { "ok", false }, { "error", "type must be 0..7 (EnemyComponent.h EnemyType)" } };

	Vec3 position{};
	if (request.contains("pos"))
		position = ReadVec(request["pos"]);
	else
	{
		const Entity player = FirstPlayer(world);
		const TransformComponent* transform = player.IsValid() ? world->GetComponent<TransformComponent>(player) : nullptr;
		if (!transform)
			return { { "ok", false }, { "error", "no pos and no player" } };
		position = transform->mWorldPosition + (request.contains("offset") ? ReadVec(request["offset"]) : Vec3(0.f, 0.f, 500.f));
	}

	// 여러 마리면 겹치지 않게 x 로 150cm 씩 벌린다.
	// SpawnEnemyAt 의 지면 보정은 ±100cm 뿐이라, 허공(예: 플레이어 앞 낭떠러지)이면 떨어져 낙사한다. 먼저 NavMesh 위로 옮긴다.
	shared_ptr<Navigation>& navigation = world->GetNavSystem();
	const int count = (std::clamp)(request.value("count", 1), 1, 20);
	json spawned = json::array();
	for (int i = 0; i < count; ++i)
	{
		Vec3 at = position + Vec3(150.f * i, 0.f, 0.f);
		if (navigation && navigation->IsInitialized() && navigation->IsPointOnNavMesh(at, 1000.f))
			at = navigation->GetNearestPointOnNavMesh(at, 1000.f);
		const Entity e = SpawnEnemyAt(world, static_cast<EnemyType>(type), at);
		if (!e.IsValid())
			continue;
		json row = { { "entity", e.GetID() }, { "pos", VecJson(at) } };
		if (const NetEntityComponent* net = world->GetComponent<NetEntityComponent>(e))
			row["netId"] = net->mNetEntityId;
		spawned.push_back(std::move(row));
	}
	return { { "ok", !spawned.empty() }, { "spawned", std::move(spawned) } };
}

json McpGod(const json& request)
{
	PlayerGodMode() = request.value("on", true);
	return { { "ok", true }, { "god", PlayerGodMode() } };
}
