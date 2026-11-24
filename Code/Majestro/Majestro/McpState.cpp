#include "pch.h"
#include "McpState.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameNames.h"
#include "TransformComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "HealthComponent.h"
#include "GameRuleComponent.h"

namespace
{
	json ToJson(const Vec3& v)
	{
		return json::array({ v.x, v.y, v.z });
	}

	template<typename T>
	T* Singleton(World* world)
	{
		return world->HasComponentPool<T>() ? world->GetSingleton<T>() : nullptr;
	}

	template<typename T>
	T* Find(World* world, Entity entity)
	{
		return world->HasComponentPool<T>() ? world->GetComponent<T>(entity) : nullptr;
	}

	json Health(World* world, Entity entity)
	{
		const HealthComponent* health = Find<HealthComponent>(world, entity);
		return health ? json{ { "hp", health->mCurrentHp }, { "maxHp", health->mMaxHp } } : json::object();
	}
}

json McpQueryState(const json& request)
{
	auto scene = gEngine->GetSceneManager().GetActiveScene();
	World* world = scene ? scene->GetWorld().get() : nullptr;
	if (world == nullptr)
		return { { "ok", false }, { "error", "no active scene" } };

	json state = { { "ok", true }, { "scene", SceneName(scene->GetSceneId()) },
		{ "entities", world->GetEntityCount() } };

	const GameRuleComponent* rule = Singleton<GameRuleComponent>(world);
	state["phase"] = PhaseName(rule ? rule->mGamePhase : 0);

	if (const GameConquestComponent* conquest = Singleton<GameConquestComponent>(world))
		state["conquest"] = { { "time", conquest->mWaveTime }, { "required", conquest->mRequiredConquestTime } };
	if (const GameEscortComponent* escort = Singleton<GameEscortComponent>(world))
		state["escort"] = { { "progress", escort->mEscortProgress }, { "stage", escort->mEscortStage },
			{ "stageCount", escort->mStageCount } };

	// ---- 로컬 플레이어 ----
	Vec3 playerPosition{};
	bool hasPlayer = false;
	if (world->HasComponentPool<LocalPlayerComponent>() && world->HasComponentPool<TransformComponent>())
	{
		auto players = world->GetEntitiesWithComponents<TransformComponent, LocalPlayerComponent>();
		if (!players.empty())
		{
			const Entity player = players[0];
			hasPlayer = true;
			playerPosition = world->GetComponent<TransformComponent>(player)->mWorldPosition;
			json p = { { "entity", player.GetID() }, { "pos", ToJson(playerPosition) } };
			p.update(Health(world, player));
			if (MainPlayerComponent* main = Find<MainPlayerComponent>(world, player))
			{
				// 클라 FSM(StateId)은 로컬 플레이어에서 쓰이지 않는다. 서버가 복제한 상태를 보낸다.
				p["action"] = main->mUpperState;	// Protocol ReplicatedActionState
				p["movement"] = main->mLowerState;	// Protocol ReplicatedMovementMode
				p["flags"] = main->mFlags;
				p["rhythmPoints"] = main->mRhythmPoints;
			}
			state["player"] = std::move(p);
		}
	}

	// ---- 적: 살아 있는 것만, 플레이어에서 가까운 순 ----
	const int limit = (std::max)(0, request.value("enemies", 20));
	struct Row { float distance; json value; };
	std::vector<Row> rows;
	if (world->HasComponentPool<EnemyComponent>() && world->HasComponentPool<TransformComponent>())
	{
		for (Entity e : world->GetEntitiesWithComponents<TransformComponent, EnemyComponent>())
		{
			const EnemyComponent* info = world->GetComponent<EnemyComponent>(e);
			const HealthComponent* health = Find<HealthComponent>(world, e);
			if (info->mAnimState == static_cast<int>(EnemyAnimState::Dead) || (health && health->IsDead()))
				continue;

			const Vec3 at = world->GetComponent<TransformComponent>(e)->mWorldPosition;
			const float distance = hasPlayer ? Vec3::Distance(at, playerPosition) : 0.f;
			json row = { { "entity", e.GetID() }, { "type", info->mEnemyType }, { "anim", info->mAnimState },
				{ "pos", ToJson(at) }, { "dist", distance } };
			row.update(Health(world, e));
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
