#pragma once

#include "Object.h"

class World;

// 길찾기 결과
struct PathResult
{
    std::vector<Vec3> waypoints;    // 경로 웨이포인트들
    bool success = false;             // 길찾기 성공 여부
    float totalDistance = 0.0f;       // 총 경로 길이
};

class NavMesh : public Object
{
public:
    NavMesh();
    ~NavMesh();
    void Load(const std::string& path);

    dtNavMesh* mDtNavMesh = nullptr;
};


class Navigation
{
public:
    Navigation();
    Navigation(World* world);
    ~Navigation();

    // NavMesh 로딩
    bool Initialize(shared_ptr<NavMesh>& navMesh);
    void Shutdown();

    // 길찾기 메인 API
    PathResult FindPath(const Vec3& start, const Vec3& end);

    // 힙 할당 없는 고정 버퍼 버전 — EnemyComponent 경로 배열에 직접 기록
    bool FindPath(const Vec3& start, const Vec3& end, Vec3* outPath, int& outCount, int maxCount);

    // 이동 벡터 검증 — 반환값: 이동 가능 거리 비율 (1.0f = 완전 통과, <1.0f = 벽에 막힘)
    // 입력 단위: NavMesh m (엔진 cm에서 /100.f 변환 후 전달)
    float Raycast(const Vec3& startM, const Vec3& endM);

    // 유틸리티 함수들
    bool IsPointOnNavMesh(const Vec3& point, float searchRadius = 5.0f);
    Vec3 GetNearestPointOnNavMesh(const Vec3& point, float searchRadius = 5.0f);
    float GetHeightAtPosition(const Vec3& position);

    // 디버그 정보
    int GetTileCount() const;
    int GetPolyCount() const;
    bool IsInitialized() const { return mInitialized; }

private:
    World* mWorld;

    // Detour 객체들
    dtNavMesh* mDtNavMesh = nullptr;
    shared_ptr<NavMesh> mNavMesh;

    dtNavMeshQuery* mNavQuery = nullptr;

    // 내부 헬퍼 함수들
    bool LoadTiledNavMesh(shared_ptr<NavMesh>& navMesh);
    dtPolyRef FindNearestPoly(const float* position, const float* extents);

    bool mInitialized = false;

    // 쿼리 설정
    static constexpr int MAX_PATH_POINTS = 256;
    static constexpr int MAX_QUERY_NODES = 2048;
};
