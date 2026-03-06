#pragma once
#include "Object.h"

class Mesh;

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
    virtual void Load(const wstring& path) override;

    dtNavMesh* mDtNavMesh = nullptr;
};


class NavigationSystem
{
public:
    NavigationSystem();
    ~NavigationSystem();

    // NavMesh 로딩
    bool Initialize(const std::string& navMeshPath);
    void Shutdown();

    // 길찾기 메인 API
    PathResult FindPath(const Vec3& start, const Vec3& end);

    // 유틸리티 함수들
    bool IsPointOnNavMesh(const Vec3& point, float searchRadius = 5.0f);
    Vec3 GetNearestPointOnNavMesh(const Vec3& point, float searchRadius = 5.0f);
    float GetHeightAtPosition(const Vec3& position);

    // 디버그 정보
    int GetTileCount() const;
    int GetPolyCount() const;
    bool IsInitialized() const { return m_initialized; }

private:
    // Detour 객체들
    dtNavMesh* mDtNavMesh;
    shared_ptr<NavMesh> mNavMesh;
    
    dtNavMeshQuery* m_navQuery = nullptr;

    // 내부 헬퍼 함수들
    bool LoadNavMeshFromFile(const std::string& filepath);
    bool LoadTiledNavMesh(const std::string& filepath);  // 타일 기반 NavMesh용
    dtPolyRef FindNearestPoly(const float* position, const float* extents);

    bool m_initialized = false;

    // 쿼리 설정
    static constexpr int MAX_PATH_POINTS = 256;
    static constexpr int MAX_QUERY_NODES = 2048;

private:
	friend class ResourceManager;
};

class NavMeshDebugRenderer
{
public:
    // dtNavMesh 표면을 삼각형 메쉬로 빌드 (디버그 가시화용)
    void BuildDebugMesh(dtNavMesh* navMesh);

    // 길찾기 경로를 빨간 선분으로 렌더링
    void RenderPath(const std::vector<Vec3>& path);

    // NavMesh 폴리곤 엣지를 흰색(NoDepth) 선분으로 렌더링 — 매 프레임 호출
    void RenderNavMesh(dtNavMesh* navMesh);

private:
    // RenderSystem::SubmitDebugLine에 위임
    void DrawLine(const Vec3& start, const Vec3& end, const Vec4& color);

    shared_ptr<Mesh> mNavMeshSurfaceMesh; // BuildDebugMesh 결과 (NavMesh 표면)
};