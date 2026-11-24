#include "pch.h"
#include "McpState.h"
#include "GameCore.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "HealthComponent.h"
#include "EnemyComponent.h"
#include "PlayerComponent.h"
#include "NetEntityComponent.h"
#include "GameRuleComponent.h"
#include "BeatSystem.h"
#include "../../Protocol/GameNames.h"

// Unity 빌드라 익명 namespace 가 다른 .cpp 와 합쳐진다. 이름이 겹치지 않게 따로 둔다.
namespace McpStateDetail
{
	json ToJson(const Vec3& v)
	{
		return json::array({ v.x, v.y, v.z });
	}

	template<typename T>
	T* Find(World* world, Entity entity)
	{
		return world->HasComponentPool<T>() ? world->GetComponent<T>(entity) : nullptr;
	}

	template<typename T>
	T* Singleton(World* world)
	{
		return world->HasComponentPool<T>() ? world->GetSingleton<T>() : nullptr;
	}

	void AddHealth(json& out, World* world, Entity entity)
	{
		if (const HealthComponent* health = Find<HealthComponent>(world, entity))
		{
			out["hp"] = health->mCurrentHp;
			out["maxHp"] = health->mMaxHp;
		}
	}

	void AddNet(json& out, World* world, Entity entity)
	{
		if (const NetEntityComponent* net = Find<NetEntityComponent>(world, entity))
		{
			out["netId"] = net->mNetEntityId;
			out["session"] = net->mSessionId;
		}
	}

	float FlatDistance(const Vec3& a, const Vec3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	uint8 Phase(World* world)
	{
		const GameRuleComponent* rule = Singleton<GameRuleComponent>(world);
		return rule ? rule->mGamePhase : 0;
	}
}

json McpListRooms()
{
	using namespace McpStateDetail;
	json rooms = json::array();
	for (const auto& [roomId, scene] : gGameCore->GetSceneManager().GetGameWorlds())
	{
		World* world = scene ? scene->GetWorld().get() : nullptr;
		if (world == nullptr)
			continue;
		const size_t players = world->HasComponentPool<MainPlayerComponent>()
			? world->GetEntitiesWithComponent<MainPlayerComponent>().size() : 0;
		rooms.push_back({ { "room", roomId }, { "scene", SceneName(scene->GetSceneId()) },
			{ "phase", PhaseName(Phase(world)) }, { "players", players } });
	}
	return { { "ok", true }, { "rooms", std::move(rooms) } };
}

json McpQueryRoomState(const json& request)
{
	using namespace McpStateDetail;
	const auto& worlds = gGameCore->GetSceneManager().GetGameWorlds();
	if (worlds.empty())
		return { { "ok", false }, { "error", "no game world (아무 방도 광장·레벨에 없음)" } };

	// room 생략 시 roomId 가 가장 작은 방
	uint32 roomId = request.contains("room") ? request["room"].get<uint32>() : UINT32_MAX;
	if (roomId == UINT32_MAX)
		for (const auto& [id, scene] : worlds)
			roomId = (std::min)(roomId, id);
	const auto it = worlds.find(roomId);
	World* world = (it != worlds.end() && it->second) ? it->second->GetWorld().get() : nullptr;
	if (world == nullptr)
		return { { "ok", false }, { "error", "no game world for room " + std::to_string(roomId) } };

	json state = { { "ok", true }, { "room", roomId }, { "scene", SceneName(it->second->GetSceneId()) },
		{ "phase", PhaseName(Phase(world)) }, { "entities", world->GetEntityCount() } };

	if (BeatSystem* beat = world->GetSystemManager()->GetSystem<BeatSystem>())
		state["songPos"] = beat->GetSongPosition();

	if (const GameConquestComponent* conquest = Singleton<GameConquestComponent>(world))
		state["conquest"] = { { "time", conquest->mWaveTime }, { "required", conquest->mRequiredConquestTime },
			{ "zone", conquest->mActiveZoneIndex }, { "wave", conquest->mWave },
			{ "playersInZone", conquest->mPlayerNum }, { "enemiesInZone", conquest->mEnemyNum } };
	if (const GameEscortComponent* escort = Singleton<GameEscortComponent>(world))
		state["escort"] = { { "progress", escort->mEscortProgress }, { "stage", escort->mEscortStage },
			{ "stageCount", escort->mStageCount } };

	// ---- 플레이어 (서버는 로컬 플레이어가 없다. 방의 플레이어 전부) ----
	json players = json::array();
	Vec3 firstPlayer{};
	bool hasPlayer = false;
	if (world->HasComponentPool<MainPlayerComponent>() && world->HasComponentPool<TransformComponent>())
	{
		for (Entity e : world->GetEntitiesWithComponents<TransformComponent, MainPlayerComponent>())
		{
			MainPlayerComponent* main = world->GetComponent<MainPlayerComponent>(e);
			const Vec3 pos = world->GetComponent<TransformComponent>(e)->mWorldPosition;
			if (!hasPlayer)
			{
				hasPlayer = true;
				firstPlayer = pos;
			}
			json p = { { "entity", e.GetID() }, { "pos", ToJson(pos) },
				{ "character", static_cast<int>(main->mPlayerType) },
				{ "action", main->GetReplicatedActionState() },		// 클라 query_state 의 action 과 같은 값
				{ "movement", main->GetReplicatedMovementMode() },	// 클라 query_state 의 movement 와 같은 값
				{ "fsmState", main->GetState() },					// 서버 FSM StateId (255 = 변환기 없음)
				{ "flags", main->mFlags }, { "rhythmPoints", main->mRhythmPoints },
				{ "lastJudgedSongPos", main->mLastJudgedInputSongPos } };
			AddHealth(p, world, e);
			AddNet(p, world, e);
			players.push_back(std::move(p));
		}
	}
	state["players"] = std::move(players);

	// ---- 적: 살아 있는 것만, 첫 플레이어에서 가까운 순 ----
	const int limit = (std::max)(0, request.value("enemies", 20));
	struct Row { float distance; json value; };
	std::vector<Row> rows;
	if (world->HasComponentPool<EnemyComponent>() && world->HasComponentPool<TransformComponent>())
	{
		for (Entity e : world->GetEntitiesWithComponents<TransformComponent, EnemyComponent>())
		{
			const EnemyComponent* info = world->GetComponent<EnemyComponent>(e);
			const HealthComponent* health = Find<HealthComponent>(world, e);
			if (info->mAnimState == static_cast<uint8>(EnemyAnimState::Dead) || (health && health->IsDead()))
				continue;
			const Vec3 at = world->GetComponent<TransformComponent>(e)->mWorldPosition;
			const float distance = hasPlayer ? FlatDistance(at, firstPlayer) : 0.f;
			json row = { { "entity", e.GetID() }, { "type", info->mEnemyType }, { "anim", info->mAnimState },
				{ "pos", ToJson(at) }, { "dist", distance } };
			AddHealth(row, world, e);
			AddNet(row, world, e);
			rows.push_back({ distance, std::move(row) });
		}
	}
	// ponytail: 매 호출 전체 정렬. 적 수백 마리면 partial_sort 로
	std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.distance < b.distance; });
	json list = json::array();
	for (size_t i = 0; i < rows.size() && i < static_cast<size_t>(limit); ++i)
		list.push_back(std::move(rows[i].value));
	state["enemies"] = { { "alive", rows.size() }, { "list", std::move(list) } };

	return state;
}
