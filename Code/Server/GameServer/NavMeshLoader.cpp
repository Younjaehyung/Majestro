#include "pch.h"
#include "NavMeshLoader.h"
#include "World.h"


NavMesh::NavMesh() : Object(OBJECT_TYPE::NAVMESH)
{
}

NavMesh::~NavMesh()
{
    if (mDtNavMesh)
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
    }
}


// RecastNavigation 데모(Sample_TileMesh)가 saveAll()로 저장한 포맷과 동일
static constexpr int NAVMESHSET_MAGIC   = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; // "MSET"
static constexpr int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader
{
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams params;
};

struct NavMeshTileHeader
{
    dtTileRef tileRef;
    int       dataSize;
};

// ========================================
// 좌표계 변환 헬퍼
// NavMesh 좌표계 (m, RH):  [0]=X, [1]=Y, [2]=Z
// 엔진 좌표계  (cm, LH):   X NavMesh[2]*100, Y NavMesh[1]*100, Z NavMesh[0]*100
// ========================================

// 엔진(cm) : NavMesh(m), 축 교환 포함
static void EngineToNavMesh(const Vec3& e, float* nm)
{
    nm[0] = e.z * 0.01f;
    nm[1] = e.y * 0.01f;
    nm[2] = e.x * 0.01f;
}

// NavMesh(m) : 엔진(cm), 축 교환 포함
static Vec3 NavMeshToEngine(const float* nm)
{
    return Vec3(nm[2] * 100.f, nm[1] * 100.f, nm[0] * 100.f);
}


void NavMesh::Load(const std::string& path)
{
    if (mDtNavMesh)
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
    }

    FILE* fp = fopen(path.c_str(), "rb");
    if (!fp)
    {
        std::cerr << "Failed to open NavMesh file: " << path << std::endl;
        return;
    }

    NavMeshSetHeader header{};
    if (fread(&header, sizeof(NavMeshSetHeader), 1, fp) != 1)
    {
        std::cerr << "Failed to read NavMesh header: " << path << std::endl;
        fclose(fp);
        return;
    }

    if (header.magic != NAVMESHSET_MAGIC)
    {
        std::cerr << "NavMesh file magic mismatch (expected MSET): " << path << std::endl;
        fclose(fp);
        return;
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        std::cerr << "NavMesh file version mismatch (expected 1, got "
                  << header.version << "): " << path << std::endl;
        fclose(fp);
        return;
    }

    mDtNavMesh = dtAllocNavMesh();
    if (!mDtNavMesh)
    {
        std::cerr << "Failed to allocate dtNavMesh." << std::endl;
        fclose(fp);
        return;
    }

    dtStatus status = mDtNavMesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
        std::cerr << "Failed to initialize dtNavMesh: " << path << std::endl;
        fclose(fp);
        return;
    }

    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader{};
        if (fread(&tileHeader, sizeof(NavMeshTileHeader), 1, fp) != 1)
            break;

        if (!tileHeader.tileRef || tileHeader.dataSize <= 0)
            break;

        unsigned char* data = static_cast<unsigned char*>(dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM));
        if (!data)
        {
            std::cerr << "Failed to allocate tile data (tile " << i << ")." << std::endl;
            break;
        }

        memset(data, 0, tileHeader.dataSize);
        if (fread(data, tileHeader.dataSize, 1, fp) != 1)
        {
            dtFree(data);
            std::cerr << "Failed to read tile data (tile " << i << ")." << std::endl;
            break;
        }

        status = mDtNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
        if (dtStatusFailed(status))
            dtFree(data);
    }

    fclose(fp);
    std::cout << "NavMesh loaded: " << header.numTiles << " tiles — " << path << std::endl;
}



Navigation::Navigation()
{
}

Navigation::Navigation(World* world) : mWorld(world)
{
}

Navigation::~Navigation()
{
    Shutdown();
}

// ========================================
// 초기화 및 종료
// ========================================

bool Navigation::Initialize(shared_ptr<NavMesh>& navMesh)
{
    if (mInitialized)
        Shutdown();

    if (!LoadTiledNavMesh(navMesh))
        return false;

    mNavQuery = dtAllocNavMeshQuery();
    if (!mNavQuery)
    {
        Shutdown();
        return false;
    }

    dtStatus status = mNavQuery->init(mDtNavMesh, MAX_QUERY_NODES);
    if (dtStatusFailed(status))
    {
        Shutdown();
        return false;
    }

    mInitialized = true;
    return true;
}

void Navigation::Shutdown()
{
    if (mNavQuery)
    {
        dtFreeNavMeshQuery(mNavQuery);
        mNavQuery = nullptr;
    }

    mNavMesh.reset();
    mDtNavMesh = nullptr;
    mInitialized = false;
}

bool Navigation::LoadTiledNavMesh(shared_ptr<NavMesh>& navMesh)
{
    mNavMesh = navMesh;

    if (!mNavMesh->mDtNavMesh)
    {
        std::cerr << "Failed to load NavMesh: mDtNavMesh is null\n";
        return false;
    }

    mDtNavMesh = mNavMesh->mDtNavMesh;
    return true;
}

// ========================================
// 내부 헬퍼
// ========================================

dtPolyRef Navigation::FindNearestPoly(const float* posNM, const float* extents)
{
    if (!mInitialized)
        return 0;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtPolyRef nearestRef = 0;
    float nearestPoint[3];
    mNavQuery->findNearestPoly(posNM, extents, &filter, &nearestRef, nearestPoint);

    return nearestRef;
}

// ========================================
// 길찾기 메인 API  (엔진 좌표 입/출력)
// ========================================

PathResult Navigation::FindPath(const Vec3& start, const Vec3& end)
{
    PathResult result;
    if (!mInitialized)
        return result;

    float startNM[3], endNM[3];
    EngineToNavMesh(start, startNM);
    EngineToNavMesh(end,   endNM);

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float extents[3] = { 2.0f, 4.0f, 2.0f };

    dtPolyRef startRef = FindNearestPoly(startNM, extents);
    dtPolyRef endRef   = FindNearestPoly(endNM,   extents);
    if (!startRef || !endRef)
        return result;

    float startPos[3], endPos[3];
    mNavQuery->closestPointOnPoly(startRef, startNM, startPos, nullptr);
    mNavQuery->closestPointOnPoly(endRef,   endNM,   endPos,   nullptr);

    dtPolyRef pathPolys[MAX_PATH_POINTS];
    int pathPolyCount = 0;
    dtStatus status = mNavQuery->findPath(
        startRef, endRef, startPos, endPos,
        &filter, pathPolys, &pathPolyCount, MAX_PATH_POINTS);

    if (dtStatusFailed(status) || pathPolyCount == 0)
        return result;

    float straightPath[MAX_PATH_POINTS * 3];
    unsigned char straightPathFlags[MAX_PATH_POINTS];
    dtPolyRef     straightPathRefs[MAX_PATH_POINTS];
    int straightPathCount = 0;
    status = mNavQuery->findStraightPath(
        startPos, endPos, pathPolys, pathPolyCount,
        straightPath, straightPathFlags, straightPathRefs,
        &straightPathCount, MAX_PATH_POINTS, DT_STRAIGHTPATH_AREA_CROSSINGS);

    if (dtStatusFailed(status) || straightPathCount == 0)
        return result;

    result.waypoints.reserve(straightPathCount);
    for (int i = 0; i < straightPathCount; ++i)
    {
        Vec3 wp = NavMeshToEngine(&straightPath[i * 3]);
        result.waypoints.push_back(wp);

        if (i > 0)
        {
            const Vec3& p1 = result.waypoints[i - 1];
            float dx = wp.x - p1.x, dy = wp.y - p1.y, dz = wp.z - p1.z;
            result.totalDistance += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
    }

    result.success = true;
    return result;
}

// 힙 할당 없는 고정 버퍼 버전 — 경로 배열에 직접 기록 (엔진 좌표 입/출력)
bool Navigation::FindPath(const Vec3& start, const Vec3& end, Vec3* outPath, int& outCount, int maxCount)
{
    outCount = 0;
    if (!mInitialized || !outPath || maxCount <= 0)
        return false;

    float startNM[3], endNM[3];
    EngineToNavMesh(start, startNM);
    EngineToNavMesh(end,   endNM);

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float extents[3] = { 2.0f, 4.0f, 2.0f };

    dtPolyRef startRef = FindNearestPoly(startNM, extents);

    // 시작점이 NavMesh 밖이면 확장 반경으로 재탐색 후 스냅
    if (!startRef)
    {
        float fallbackExtents[3] = { 10.0f, 20.0f, 10.0f };
        startRef = FindNearestPoly(startNM, fallbackExtents);
        if (!startRef)
            return false;

        mNavQuery->closestPointOnPoly(startRef, startNM, startNM, nullptr);
    }

    dtPolyRef endRef = FindNearestPoly(endNM, extents);
    if (!endRef)
        return false;

    float startPos[3], endPos[3];
    mNavQuery->closestPointOnPoly(startRef, startNM, startPos, nullptr);
    mNavQuery->closestPointOnPoly(endRef,   endNM,   endPos,   nullptr);

    dtPolyRef pathPolys[MAX_PATH_POINTS];
    int pathPolyCount = 0;
    dtStatus status = mNavQuery->findPath(
        startRef, endRef, startPos, endPos,
        &filter, pathPolys, &pathPolyCount, MAX_PATH_POINTS);

    if (dtStatusFailed(status) || pathPolyCount == 0)
        return false;

    float straightPath[MAX_PATH_POINTS * 3];
    unsigned char straightPathFlags[MAX_PATH_POINTS];
    dtPolyRef     straightPathRefs[MAX_PATH_POINTS];
    int straightPathCount = 0;
    status = mNavQuery->findStraightPath(
        startPos, endPos, pathPolys, pathPolyCount,
        straightPath, straightPathFlags, straightPathRefs,
        &straightPathCount, MAX_PATH_POINTS, DT_STRAIGHTPATH_AREA_CROSSINGS);

    if (dtStatusFailed(status) || straightPathCount == 0)
        return false;

    const int writeCount = min(straightPathCount, maxCount);
    for (int i = 0; i < writeCount; ++i)
        outPath[i] = NavMeshToEngine(&straightPath[i * 3]);

    outCount = writeCount;
    return true;
}

// 이동 벡터 검증 — 반환: 이동 가능 거리 비율 (1.0f = 통과, <1.0f = 벽에 막힘)
// start/end: 엔진 좌표 (cm) — 내부에서 NavMesh 좌표로 변환
float Navigation::Raycast(const Vec3& start, const Vec3& end)
{
    if (!mInitialized)
        return 1.0f;

    float startNM[3], endNM[3];
    EngineToNavMesh(start, startNM);
    EngineToNavMesh(end,   endNM);

    // Y extents 크게 -> 플레이어가 약간 공중에 있어도 바닥 폴리곤 탐색
    float extents[3] = { 0.5f, 4.0f, 0.5f };
    dtPolyRef startRef = FindNearestPoly(startNM, extents);
    if (!startRef)
        return 1.0f; // NavMesh 밖 -> 검증 스킵

    float snappedStart[3];
    mNavQuery->closestPointOnPoly(startRef, startNM, snappedStart, nullptr);

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtRaycastHit hit{};
    hit.path    = nullptr;
    hit.maxPath = 0;

    dtStatus status = mNavQuery->raycast(
        startRef, snappedStart, endNM,
        &filter, 0, &hit);

    if (dtStatusFailed(status))
        return 1.0f;

    return (hit.t < 1.0f) ? hit.t : 1.0f;
}

// NavMesh 표면을 따라 start->end 이동, 도달 가능한 XZ 위치를 outResult로 반환
// Y는 start.y를 그대로 유지 (중력 시스템 위임)
bool Navigation::MoveAlongSurface(const Vec3& start, const Vec3& end, Vec3& outResult)
{
    if (!mInitialized)
        return false;

    float startNM[3], endNM[3];
    EngineToNavMesh(start, startNM);
    EngineToNavMesh(end,   endNM);

    float extents[3] = { 2.0f, 4.0f, 2.0f };
    dtPolyRef startRef = FindNearestPoly(startNM, extents);
    if (!startRef)
        return false; // NavMesh 밖 - 검증 스킵 (이동 허용)

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float resultNM[3];
    static constexpr int MAX_VISITED = 16;
    dtPolyRef visited[MAX_VISITED];
    int visitedCount = 0;

    dtStatus status = mNavQuery->moveAlongSurface(
        startRef, startNM, endNM,
        &filter, resultNM, visited, &visitedCount, MAX_VISITED);

    if (dtStatusFailed(status))
        return false;

    // XZ만 NavMesh 결과로 교체 — Y는 엔진 중력 시스템이 관리
    Vec3 engineResult = NavMeshToEngine(resultNM);
    outResult.x = engineResult.x;
    outResult.y = start.y;          // Y 유지
    outResult.z = engineResult.z;
    return true;
}

// ========================================
// 유틸리티 함수들 (엔진 좌표 입력)
// ========================================

bool Navigation::IsPointOnNavMesh(const Vec3& point, float searchRadius)
{
    if (!mInitialized)
        return false;

    float posNM[3];
    EngineToNavMesh(point, posNM);

    const float r = searchRadius * 0.01f; // cm -> m
    float extents[3] = { r, r * 2.0f, r };
    return FindNearestPoly(posNM, extents) != 0;
}

Vec3 Navigation::GetNearestPointOnNavMesh(const Vec3& point, float searchRadius)
{
    if (!mInitialized)
        return point;

    float posNM[3];
    EngineToNavMesh(point, posNM);

    const float r = searchRadius * 0.01f;
    float extents[3] = { r, r * 2.0f, r };
    dtPolyRef polyRef = FindNearestPoly(posNM, extents);

    if (polyRef)
    {
        float nearest[3];
        mNavQuery->closestPointOnPoly(polyRef, posNM, nearest, nullptr);
        return NavMeshToEngine(nearest);
    }

    return point;
}

float Navigation::GetHeightAtPosition(const Vec3& position)
{
    if (!mInitialized)
        return position.y;

    float posNM[3];
    EngineToNavMesh(position, posNM);

    float extents[3] = { 0.01f, 0.1f, 0.01f }; // 1cm, 10cm, 1cm
    dtPolyRef polyRef = FindNearestPoly(posNM, extents);

    if (polyRef)
    {
        float heightNM;
        mNavQuery->getPolyHeight(polyRef, posNM, &heightNM);
        return heightNM * 100.f; // m -> cm
    }

    return position.y;
}

// ========================================
// 디버그 정보
// ========================================

int Navigation::GetTileCount() const
{
    if (!mDtNavMesh)
        return 0;

    int count = 0;
    int maxTiles = mDtNavMesh->getMaxTiles();
    for (int i = 0; i < maxTiles; ++i)
    {
        const dtMeshTile* tile = static_cast<const dtNavMesh*>(mDtNavMesh)->getTile(i);
        if (tile && tile->header)
            count++;
    }
    return count;
}

int Navigation::GetPolyCount() const
{
    if (!mDtNavMesh)
        return 0;

    int total = 0;
    int maxTiles = mDtNavMesh->getMaxTiles();
    for (int i = 0; i < maxTiles; ++i)
    {
        const dtMeshTile* tile = static_cast<const dtNavMesh*>(mDtNavMesh)->getTile(i);
        if (tile && tile->header)
            total += tile->header->polyCount;
    }
    return total;
}
