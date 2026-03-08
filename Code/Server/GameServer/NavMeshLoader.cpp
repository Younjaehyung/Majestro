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
    dtTileRef tileRef;  // 원본 tileRef 복원용
    int       dataSize;
};

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

    // 1. 헤더 읽기 (magic + version + numTiles + dtNavMeshParams)
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

    // 2. NavMesh 객체 생성 및 초기화
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

    // 3. 타일 데이터 로드
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

        // tileRef를 복원해 addTile — 폴리곤 참조 ID가 저장 당시와 동일하게 유지됨
        status = mDtNavMesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
        if (dtStatusFailed(status))
        {
            dtFree(data);
            // 일부 타일 손상 가능성 — 계속 진행
        }
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

bool Navigation::Initialize(shared_ptr<NavMesh>& navMesh/*const std::string& navMeshPath*/)
{
    if (mInitialized)
    {
        Shutdown();
    }


    // 1. NavMesh 파일 로드
    if (!LoadTiledNavMesh(navMesh))
    {
        return false;
    }

    // 2. NavMeshQuery 생성 (길찾기용)
    mNavQuery = dtAllocNavMeshQuery();
    if (!mNavQuery)
    {
        Shutdown();
        return false;
    }

    // 3. NavMeshQuery 초기화
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

    // mNavMesh 소멸자가 dtFreeNavMesh를 처리하므로 mDtNavMesh는 직접 해제하지 않음
    mNavMesh.reset();
    mDtNavMesh = nullptr;

    mInitialized = false;
}

// ========================================
// NavMesh 파일 로딩
// ========================================

bool Navigation::LoadTiledNavMesh(shared_ptr<NavMesh>& navMesh /*const std::string& filepath*/)
{
   
    mNavMesh = navMesh;

    if (!mNavMesh->mDtNavMesh)
    {
        std::cerr << "Failed to load NavMesh from path: LoadTiledNavMesh"  << "\n";
        return false;
    }

    mDtNavMesh = mNavMesh->mDtNavMesh;
    return true;
}

// ========================================
// 길찾기 메인 API
// ========================================

PathResult Navigation::FindPath(const Vec3& start, const Vec3& end)
{
    PathResult result;

    if (!mInitialized)
    {
        return result;
    }

    // 1. 쿼리 필터 설정 (어떤 폴리곤을 통과할 수 있는지)
    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);  // 모든 플래그 포함
    filter.setExcludeFlags(0);        // 제외 없음

    // 2. 검색 범위 설정 (시작/끝점 주변 검색 박스)
    float extents[3] = { 2.0f, 4.0f, 2.0f };  // X, Y, Z 반경 (미터)

    // 3. 시작/끝 위치에서 가장 가까운 NavMesh 폴리곤 찾기
    dtPolyRef startRef = FindNearestPoly(&start.x, extents);
    dtPolyRef endRef = FindNearestPoly(&end.x, extents);

    if (!startRef || !endRef)
    {
        // NavMesh에서 너무 멀리 떨어진 위치
        return result;
    }

    // 4. NavMesh 상의 실제 시작/끝 점 찾기
    float startPos[3], endPos[3];
    mNavQuery->closestPointOnPoly(startRef, &start.x, startPos, nullptr);
    mNavQuery->closestPointOnPoly(endRef, &end.x, endPos, nullptr);

    // 5. A* 길찾기 실행 (폴리곤 경로 찾기)
    dtPolyRef pathPolys[MAX_PATH_POINTS];
    int pathPolyCount = 0;

    dtStatus status = mNavQuery->findPath(
        startRef, endRef,
        startPos, endPos,
        &filter,
        pathPolys, &pathPolyCount, MAX_PATH_POINTS
    );

    if (dtStatusFailed(status) || pathPolyCount == 0)
    {
        return result;
    }

    // 6. 폴리곤 경로를 직선 경로로 변환 (웨이포인트 생성)
    float straightPath[MAX_PATH_POINTS * 3];
    unsigned char straightPathFlags[MAX_PATH_POINTS];
    dtPolyRef straightPathRefs[MAX_PATH_POINTS];
    int straightPathCount = 0;

    status = mNavQuery->findStraightPath(
        startPos, endPos,
        pathPolys, pathPolyCount,
        straightPath,
        straightPathFlags,
        straightPathRefs,
        &straightPathCount,
        MAX_PATH_POINTS,
        DT_STRAIGHTPATH_AREA_CROSSINGS  // 영역 교차점 포함
    );

    if (dtStatusFailed(status) || straightPathCount == 0)
    {
        return result;
    }

    // 7. 결과 변환 (float 배열 → Vec3 벡터)
    result.waypoints.reserve(straightPathCount);
    for (int i = 0; i < straightPathCount; ++i)
    {
        Vec3 waypoint(
            straightPath[i * 3 + 0],
            straightPath[i * 3 + 1],
            straightPath[i * 3 + 2]
        );
        result.waypoints.push_back(waypoint);
    }

    // 8. 총 경로 길이 계산
    result.totalDistance = 0.0f;
    for (size_t i = 1; i < result.waypoints.size(); ++i)
    {
        const Vec3& p1 = result.waypoints[i - 1];
        const Vec3& p2 = result.waypoints[i];

        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dz = p2.z - p1.z;
        result.totalDistance += std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    result.success = true;
    return result;
}

// 고정 버퍼 — EnemyMovementComponent 경로 배열에 직접 기록
bool Navigation::FindPath(const Vec3& start, const Vec3& end, Vec3* outPath, int& outCount, int maxCount)
{
    outCount = 0;

    if (!mInitialized || !outPath || maxCount <= 0)
        return false;

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    float extents[3] = { 2.0f, 4.0f, 2.0f };

    dtPolyRef startRef = FindNearestPoly(&start.x, extents);

    // Enemy가 NavMesh 밖에 있으면 확장 반경으로 재탐색 후 가장 가까운 폴리곤에 스냅
    float snappedStart[3] = { start.x, start.y, start.z };
    if (!startRef)
    {
        float fallbackExtents[3] = { 10.0f, 20.0f, 10.0f };
        startRef = FindNearestPoly(snappedStart, fallbackExtents);
        if (!startRef)
            return false; // NavMesh와 너무 멀리 떨어짐 → 폴백으로 위임

        // 스냅: NavMesh 위의 가장 가까운 점을 실제 시작점으로 사용
        mNavQuery->closestPointOnPoly(startRef, snappedStart, snappedStart, nullptr);
    }

    dtPolyRef endRef = FindNearestPoly(&end.x, extents);
    if (!endRef)
        return false;

    float startPos[3], endPos[3];
    mNavQuery->closestPointOnPoly(startRef, snappedStart, startPos, nullptr);
    mNavQuery->closestPointOnPoly(endRef,   &end.x,       endPos,   nullptr);

    dtPolyRef pathPolys[MAX_PATH_POINTS];
    int pathPolyCount = 0;

    dtStatus status = mNavQuery->findPath(
        startRef, endRef,
        startPos, endPos,
        &filter,
        pathPolys, &pathPolyCount, MAX_PATH_POINTS
    );

    if (dtStatusFailed(status) || pathPolyCount == 0)
        return false;

    // straightPath는 스택에 고정 할당 — 힙 사용 없음
    float straightPath[MAX_PATH_POINTS * 3];
    unsigned char straightPathFlags[MAX_PATH_POINTS];
    dtPolyRef     straightPathRefs[MAX_PATH_POINTS];
    int straightPathCount = 0;

    status = mNavQuery->findStraightPath(
        startPos, endPos,
        pathPolys, pathPolyCount,
        straightPath, straightPathFlags, straightPathRefs,
        &straightPathCount, MAX_PATH_POINTS,
        DT_STRAIGHTPATH_AREA_CROSSINGS
    );

    if (dtStatusFailed(status) || straightPathCount == 0)
        return false;

    const int writeCount = min(straightPathCount, maxCount);
    for (int i = 0; i < writeCount; ++i)
    {
        outPath[i].x = straightPath[i * 3 + 0] * 100.f;
        outPath[i].y = straightPath[i * 3 + 1] * 100.f;
        outPath[i].z = straightPath[i * 3 + 2] * 100.f;
    }
    outCount = writeCount;

    return true;
}

// 이동 벡터 NavMesh 검증 — 반환: 이동 가능 거리 비율
// startM/endM: NavMesh m 단위 (엔진 좌표 / 100.f)
float Navigation::Raycast(const Vec3& startM, const Vec3& endM)
{
    if (!mInitialized)
        return 1.0f;

    // 시작점 근처 폴리곤 탐색 (Y extents 크게 → 공중에 있어도 바닥 폴리곤 탐색)
    float extents[3] = { 0.5f, 4.0f, 0.5f };
    dtPolyRef startRef = FindNearestPoly(&startM.x, extents);
    if (!startRef)
        return 1.0f; // NavMesh 밖 → 검증 스킵

    // 실제 폴리곤 위 스냅 좌표를 raycast 시작점으로 사용
    float snappedStart[3];
    mNavQuery->closestPointOnPoly(startRef, &startM.x, snappedStart, nullptr);

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtRaycastHit hit{};
    hit.path    = nullptr;
    hit.maxPath = 0;

    dtStatus status = mNavQuery->raycast(
        startRef,
        snappedStart, &endM.x,
        &filter, 0,
        &hit
    );

    if (dtStatusFailed(status))
        return 1.0f;

    // hit.t: FLT_MAX = 막힘 없음, 0~1 = 비율만큼만 통과
    return (hit.t < 1.0f) ? hit.t : 1.0f;
}

// ========================================
// 유틸리티 함수들
// ========================================

// 가장 가까운 NavMesh 폴리곤 찾기
dtPolyRef Navigation::FindNearestPoly(const float* position, const float* extents)
{
    if (!mInitialized)
    {
        return 0;
    }

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtPolyRef nearestRef = 0;
    float nearestPoint[3];

    mNavQuery->findNearestPoly(position, extents, &filter, &nearestRef, nearestPoint);

    return nearestRef;
}

// 점이 NavMesh 위에 있는지 확인
bool Navigation::IsPointOnNavMesh(const Vec3& point, float searchRadius)
{
    if (!mInitialized)
    {
        return false;
    }

    float extents[3] = { searchRadius, searchRadius * 2.0f, searchRadius };
    dtPolyRef polyRef = FindNearestPoly(&point.x, extents);

    return polyRef != 0;
}

// NavMesh 상의 가장 가까운 점 반환
Vec3 Navigation::GetNearestPointOnNavMesh(const Vec3& point, float searchRadius)
{
    Vec3 result = point;

    if (!mInitialized)
    {
        return result;
    }

    float extents[3] = { searchRadius, searchRadius * 2.0f, searchRadius };
    dtPolyRef polyRef = FindNearestPoly(&point.x, extents);

    if (polyRef)
    {
        float nearest[3];
        mNavQuery->closestPointOnPoly(polyRef, &point.x, nearest, nullptr);

        result.x = nearest[0];
        result.y = nearest[1];
        result.z = nearest[2];
    }

    return result;
}

// 특정 위치의 NavMesh 높이 가져오기
float Navigation::GetHeightAtPosition(const Vec3& position)
{
    if (!mInitialized)
    {
        return position.y;
    }

    float extents[3] = { 1.0f, 10.0f, 1.0f };
    dtPolyRef polyRef = FindNearestPoly(&position.x, extents);

    if (polyRef)
    {
        float height;
        mNavQuery->getPolyHeight(polyRef, &position.x, &height);
        return height;
    }

    return position.y;
}

// ========================================
// 디버그 정보
// ========================================

int Navigation::GetTileCount() const
{
    if (!mDtNavMesh)
    {
        return 0;
    }

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
    {
        return 0;
    }

    int totalPolyCount = 0;
    int maxTiles = mDtNavMesh->getMaxTiles();

    for (int i = 0; i < maxTiles; ++i)
    {
        const dtMeshTile* tile = static_cast<const dtNavMesh*>(mDtNavMesh)->getTile(i);
        if (tile && tile->header)
            totalPolyCount += tile->header->polyCount;
    }

    return totalPolyCount;
}
