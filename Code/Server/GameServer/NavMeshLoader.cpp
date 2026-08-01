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
        MJLOG_ERROR(ResourceLoad, "NavMesh 열기 실패 path={}", path);
        return;
    }

    NavMeshSetHeader header{};
    if (fread(&header, sizeof(NavMeshSetHeader), 1, fp) != 1)
    {
        MJLOG_ERROR(ResourceLoad, "NavMesh 헤더 읽기 실패 path={}", path);
        fclose(fp);
        return;
    }

    if (header.magic != NAVMESHSET_MAGIC)
    {
        MJLOG_ERROR(ResourceLoad, "NavMesh magic 불일치(MSET 기대) path={}", path);
        fclose(fp);
        return;
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        MJLOG_ERROR(ResourceLoad, "NavMesh 버전 불일치 expected={} got={} path={}",
            NAVMESHSET_VERSION, header.version, path);
        fclose(fp);
        return;
    }

    mDtNavMesh = dtAllocNavMesh();
    if (!mDtNavMesh)
    {
        MJLOG_ERROR(ResourceLoad, "dtNavMesh 할당 실패 path={}", path);
        fclose(fp);
        return;
    }

    dtStatus status = mDtNavMesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
        MJLOG_ERROR(ResourceLoad, "dtNavMesh 초기화 실패 path={}", path);
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
            MJLOG_ERROR(ResourceLoad, "NavMesh 타일 할당 실패 tile={} path={}", i, path);
            break;
        }

        memset(data, 0, tileHeader.dataSize);
        if (fread(data, tileHeader.dataSize, 1, fp) != 1)
        {
            dtFree(data);
            MJLOG_ERROR(ResourceLoad, "NavMesh 타일 읽기 실패 tile={} path={}", i, path);
            break;
        }

        status = mDtNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
        if (dtStatusFailed(status))
            dtFree(data);
    }

    fclose(fp);
    MJLOG_INFO(ResourceLoad, "NavMesh 로드 완료 tiles={} path={}", header.numTiles, path);
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
        MJLOG_ERROR(ResourceLoad, "NavMesh 로드 실패 — mDtNavMesh 가 null");
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

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    // findNearestPoly를 직접 호출해 nearestPt(폴리곤 위의 가장 가까운 점)도 함께 얻음
    float extents[3] = { 2.0f, 4.0f, 2.0f };
    dtPolyRef startRef = 0;
    float nearestPt[3];
    mNavQuery->findNearestPoly(startNM, extents, &filter, &startRef, nearestPt);
    if (!startRef)
        return false; // NavMesh 밖

    /*탐색 기준점으로부터 AABB 박스 안에서 가장 가까운 폴리곤 하나를 반환
        - nearestPt는 폴리곤 표면 위의 점 == moveAlongSurface의 시작점으로 사용할 수 있음*/
    //mNavQuery->findNearestPoly(
    //    startNM,       // 탐색 기준점 (NavMesh 좌표)
    //    extents,       // 탐색 박스 반경 [X, Y, Z] (m)
    //    &filter,       // 이동 가능 플래그 필터
    //    &startRef,     // OUT: 발견된 폴리곤 레퍼런스
    //    nearestPt      // OUT: 그 폴리곤 위의 가장 가까운 점 (3D)
    //);

    // XZ 스냅 거리 확인:
    // nearestPt가 startNM에서 0.5m(=50cm) 이상 떨어졌으면 플레이어가 NavMesh 밖에 있다는 뜻
    // → 스냅된 위치에서 moveAlongSurface를 돌리면 전혀 다른 좌표가 반환되어 순간이동 발생
    // NavMesh 좌표계: [0]=EngineZ, [2]=EngineX
    const float dz = nearestPt[0] - startNM[0];
    const float dx = nearestPt[2] - startNM[2];
    if (dz * dz + dx * dx > 0.5f * 0.5f)
        return false; // NavMesh 벗어남

    float resultNM[3];
    static constexpr int MAX_VISITED = 16;
    dtPolyRef visited[MAX_VISITED];
    int visitedCount = 0;

    // nearestPt는 폴리곤 위의 점이므로 Detour moveAlongSurface API 조건 충족
    dtStatus status = mNavQuery->moveAlongSurface(
        startRef, nearestPt, endNM,
        &filter, resultNM, visited, &visitedCount, MAX_VISITED);

    //NavMesh 표면을 따라 startPos에서 endPos로 이동하다 벽 / 경계에 막히면 그 직전에서 멈춤
    //    - resultNM의 Y는 moveAlongSurface가 이동 경로 중 마지막으로 접촉한 폴리곤 표면의 Y값
    //mNavQuery->moveAlongSurface(
    //    startRef,      // 시작 폴리곤 레퍼런스 (startPos가 이 폴리곤 안에 있어야 함)
    //    nearestPt,     // 시작 위치 (NavMesh 좌표, 반드시 폴리곤 내부)
    //    endNM,         // 목표 위치 (NavMesh 좌표)
    //    &filter,       // 필터
    //    resultNM,      // OUT: 실제로 도달한 위치
    //    visited,       // OUT: 통과한 폴리곤 목록
    //    &visitedCount, // OUT: 통과한 폴리곤 수
    //    MAX_VISITED    // visited 배열 최대 크기
    //);

    if (dtStatusFailed(status))
        return false;


    // visited 배열의 마지막 폴리곤(= 결과 위치가 속한 폴리곤)
    dtPolyRef resultRef = (visitedCount > 0) ? visited[visitedCount - 1] : startRef;
    float polyHeight = resultNM[1];
    mNavQuery->getPolyHeight(resultRef, resultNM, &polyHeight);
    //폴리곤 내부의 임의 XZ 좌표에서 삼각형 보간으로 정확한 높이를 계산
    //
    //   mNavQuery->getPolyHeight(
    //  ref,           // 폴리곤 레퍼런스
    //  pos,           // XZ 위치 (이 폴리곤 안이어야 함)
    //  &height        // OUT: 해당 XZ에서의 폴리곤 내부 보간 Y
    //    );
    //
    //   findNearestPoly나 moveAlongSurface의 Y보다 더 정확함
    resultNM[1] = polyHeight;

    outResult = NavMeshToEngine(resultNM);
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

    float extents[3] = { 0.5f, 4.0f, 0.5f }; // XZ ±0.5m, Y ±4m (NavMesh space)
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