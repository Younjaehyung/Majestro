#pragma once
#include "World.h"
#include "System.h"
#include "EnemyComponent.h"
#include <unordered_map>

class Navigation;
class NavMesh;
class EventManager;
class EnemyMovementComponent;

class EnemySystem : public System
{
public:
    EnemySystem(World* world);

    void Initialize();

    void Update(float deltaTime) override;

    Vec3 PathFinder(const Vec3& from);  // 가장 가까운 플레이어 위치 탐색

private:
    void UpdateOnnxToggle();

    void UpdateDetourState(
        EnemyMovementComponent* movementComp,
        const Vec3& myPos,
        const Vec3& desiredTarget,
        float dt);

    bool ShouldEnterDetour(
        const EnemyMovementComponent* movementComp,
        const Vec3& myPos,
        const Vec3& desiredTarget) const;

    bool TryComputeOnnxBaseMoveTarget(
        const Entity& entity,
        const Vec3& myPos,
        const Vec3& playerPos,
        const std::shared_ptr<Navigation>& navSystem,
        Vec3& outTarget) const;

    bool HandleAttackState(
        const Entity& entity,
        EnemyComponent* enemyComp,
        EnemyMovementComponent* movementComp,
        float nearestPlayerDistSq,
        float beatSeconds,
        float nowSeconds,
        const std::shared_ptr<EventManager>& eventManager);

    void HandleRunState(
        const Entity& entity,
        EnemyComponent* enemyComp,
        EnemyMovementComponent* movementComp,
        const Vec3& myPos,
        const Vec3& playerPos,
        const std::shared_ptr<Navigation>& navSystem,
        float nowSeconds,
        float dt,
        int entityIndex);

    void HaltByState(EnemyComponent* enemyComp, EnemyMovementComponent* movementComp, EnemyAnimState state);

    std::vector<Vec3> mPlayerPositions;

    std::shared_ptr<NavMesh> mNavMesh; // NavMesh 리소스 생존 보장용
    bool mUseOnnxBaseMove = true;
    bool mOnnxToggleKeyHeld = false;

    // 임계값 상수
    static constexpr float ARRIVE_THRESHOLD_SQ = 4.0f * 100.f;    // 웨이포인트 도달 반경 (2m)
    static constexpr float RETARGET_THRESHOLD_SQ = 100.0f * 100.f; // 목적지 변경 감지 거리 (10m)
    static constexpr float NEARBY_ENEMY_RADIUS = 1000.0f;
    static constexpr size_t MAX_NEARBY_ENEMIES = 3;
    static constexpr float ONNX_MAP_RANGE = 6000.0f;
    static constexpr float ONNX_SENSE_RADIUS = 1000.0f;
    static constexpr float ONNX_TACTICAL_TARGET_RADIUS = 600.0f;
    static constexpr float DETOUR_EXIT_DISTANCE = 1000.0f;
    static constexpr float DETOUR_TIMEOUT_SECONDS = 5.0f;
    static constexpr float STUCK_PROGRESS_WINDOW = 0.6f;
    static constexpr float STUCK_PROGRESS_DISTANCE = 50.0f;
    static constexpr float STUCK_TARGET_DISTANCE = 250.0f;
    static constexpr float STUCK_TRIGGER_SECONDS = 0.8f;
};
