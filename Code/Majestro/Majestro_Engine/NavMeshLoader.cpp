#include "pch.h"
#include "NavMeshLoader.h"

#include "Engine.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "RenderSystem.h"



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

void NavMesh::Load(const wstring& path)
{
    if (mDtNavMesh)
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
    }

    // std::ifstream 대신 fopen 사용 — 멀티바이트 경로 문제 회피
    FILE* fp = _wfopen(path.c_str(), L"rb");
    if (!fp)
    {
        std::cerr << "Failed to open NavMesh file: " << ws2s(path) << std::endl;
        return;
    }

    // 1. 헤더 읽기 (magic + version + numTiles + dtNavMeshParams)
    NavMeshSetHeader header{};
    if (fread(&header, sizeof(NavMeshSetHeader), 1, fp) != 1)
    {
        std::cerr << "Failed to read NavMesh header: " << ws2s(path) << std::endl;
        fclose(fp);
        return;
    }

    if (header.magic != NAVMESHSET_MAGIC)
    {
        std::cerr << "NavMesh file magic mismatch (expected MSET): " << ws2s(path) << std::endl;
        fclose(fp);
        return;
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        std::cerr << "NavMesh file version mismatch (expected 1, got "
                  << header.version << "): " << ws2s(path) << std::endl;
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
        std::cerr << "Failed to initialize dtNavMesh: " << ws2s(path) << std::endl;
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
    std::cout << "NavMesh loaded: " << header.numTiles << " tiles — " << ws2s(path) << std::endl;
}



NavigationSystem::NavigationSystem()
{
}

NavigationSystem::~NavigationSystem()
{
    Shutdown();
}

// ========================================
// 초기화 및 종료
// ========================================

bool NavigationSystem::Initialize(const std::string& navMeshPath)
{
    if (m_initialized)
    {
        Shutdown();
    }

    // 1. NavMesh 파일 로드
    if (!LoadTiledNavMesh(navMeshPath))
    {
        return false;
    }

    // 2. NavMeshQuery 생성 (길찾기용)
    m_navQuery = dtAllocNavMeshQuery();
    if (!m_navQuery)
    {
        Shutdown();
        return false;
    }

    // 3. NavMeshQuery 초기화
    dtStatus status = m_navQuery->init(mDtNavMesh, MAX_QUERY_NODES);
    if (dtStatusFailed(status))
    {
        Shutdown();
        return false;
    }

    m_initialized = true;
    return true;
}

void NavigationSystem::Shutdown()
{
    if (m_navQuery)
    {
        dtFreeNavMeshQuery(m_navQuery);
        m_navQuery = nullptr;
    }

    if (mDtNavMesh)
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
    }

    m_initialized = false;
}

// ========================================
// NavMesh 파일 로딩 (타일 기반)
// ========================================

bool NavigationSystem::LoadTiledNavMesh(const std::string& filepath)
{
    mNavMesh = RESOURCEMANAGER.Get<NavMesh>(L"NavMesh");
    if(mNavMesh == nullptr)
    {
        mNavMesh = RESOURCEMANAGER.LoadNavMesh(s2ws(filepath));
        if(mNavMesh == nullptr)
        {
            std::cerr << "Failed to load NavMesh from path: " << filepath << "\n";
            return false;
		}
	}
	mDtNavMesh = mNavMesh->mDtNavMesh;

    return true;





    // 기존 코드들

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // 1. NavMesh 파라미터 읽기
    dtNavMeshParams params;
    file.read(reinterpret_cast<char*>(&params), sizeof(dtNavMeshParams));

    // 2. NavMesh 객체 생성
    mDtNavMesh = dtAllocNavMesh();
    if (!mDtNavMesh)
    {
        return false;
    }

    dtStatus status = mDtNavMesh->init(&params);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(mDtNavMesh);
        mDtNavMesh = nullptr;
        return false;
    }

    // 3. 타일 개수 읽기
    int tileCount = 0;
    file.read(reinterpret_cast<char*>(&tileCount), sizeof(int));

    // 4. 각 타일 데이터 로드
    for (int i = 0; i < tileCount; ++i)
    {
        // 타일 좌표 읽기
        int tileX, tileY, tileLayer;
        file.read(reinterpret_cast<char*>(&tileX), sizeof(int));
        file.read(reinterpret_cast<char*>(&tileY), sizeof(int));
        file.read(reinterpret_cast<char*>(&tileLayer), sizeof(int));

        // 타일 데이터 크기 읽기
        int dataSize = 0;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(int));

        // 타일 데이터 읽기
        unsigned char* data = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        file.read(reinterpret_cast<char*>(data), dataSize);

        // NavMesh에 타일 추가
        status = mDtNavMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, nullptr);
        if (dtStatusFailed(status))
        {
            dtFree(data);
            // 실패해도 계속 진행 (일부 타일 손상 가능성)
        }
    }

    file.close();
    return true;
}

// ========================================
// 길찾기 메인 API
// ========================================

PathResult NavigationSystem::FindPath(const Vec3& start, const Vec3& end)
{
    PathResult result;

    if (!m_initialized)
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
    m_navQuery->closestPointOnPoly(startRef, &start.x, startPos, nullptr);
    m_navQuery->closestPointOnPoly(endRef, &end.x, endPos, nullptr);

    // 5. A* 길찾기 실행 (폴리곤 경로 찾기)
    dtPolyRef pathPolys[MAX_PATH_POINTS];
    int pathPolyCount = 0;

    dtStatus status = m_navQuery->findPath(
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

    status = m_navQuery->findStraightPath(
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

// ========================================
// 유틸리티 함수들
// ========================================

// 가장 가까운 NavMesh 폴리곤 찾기
dtPolyRef NavigationSystem::FindNearestPoly(const float* position, const float* extents)
{
    if (!m_initialized)
    {
        return 0;
    }

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtPolyRef nearestRef = 0;
    float nearestPoint[3];

    m_navQuery->findNearestPoly(position, extents, &filter, &nearestRef, nearestPoint);

    return nearestRef;
}

// 점이 NavMesh 위에 있는지 확인
bool NavigationSystem::IsPointOnNavMesh(const Vec3& point, float searchRadius)
{
    if (!m_initialized)
    {
        return false;
    }

    float extents[3] = { searchRadius, searchRadius * 2.0f, searchRadius };
    dtPolyRef polyRef = FindNearestPoly(&point.x, extents);

    return polyRef != 0;
}

// NavMesh 상의 가장 가까운 점 반환
Vec3 NavigationSystem::GetNearestPointOnNavMesh(const Vec3& point, float searchRadius)
{
    Vec3 result = point;

    if (!m_initialized)
    {
        return result;
    }

    float extents[3] = { searchRadius, searchRadius * 2.0f, searchRadius };
    dtPolyRef polyRef = FindNearestPoly(&point.x, extents);

    if (polyRef)
    {
        float nearest[3];
        m_navQuery->closestPointOnPoly(polyRef, &point.x, nearest, nullptr);

        result.x = nearest[0];
        result.y = nearest[1];
        result.z = nearest[2];
    }

    return result;
}

// 특정 위치의 NavMesh 높이 가져오기
float NavigationSystem::GetHeightAtPosition(const Vec3& position)
{
    if (!m_initialized)
    {
        return position.y;
    }

    float extents[3] = { 1.0f, 10.0f, 1.0f };
    dtPolyRef polyRef = FindNearestPoly(&position.x, extents);

    if (polyRef)
    {
        float height;
        m_navQuery->getPolyHeight(polyRef, &position.x, &height);
        return height;
    }

    return position.y;
}

// ========================================
// 디버그 정보 - 수정된 버전
// ========================================

// NavigationSystem.cpp - 완전히 재작성된 디버그 함수

int NavigationSystem::GetTileCount() const
{
    if (!mDtNavMesh)
    {
        return 0;
    }

    int count = 0;
    const dtNavMeshParams* params = mDtNavMesh->getParams();

    // 전체 월드 영역에서 가능한 타일 범위 계산
    // 주의: 실제 맵 크기를 알아야 정확함
    int maxTilesX = 32; // 예시: 실제 맵에 맞게 조정 필요
    int maxTilesY = 32;

    for (int y = 0; y < maxTilesY; ++y)
    {
        for (int x = 0; x < maxTilesX; ++x)
        {
            const dtMeshTile* tile = mDtNavMesh->getTileAt(x, y, 0);
            if (tile && tile->header)
            {
                count++;
            }
        }
    }

    return count;
}
int NavigationSystem::GetPolyCount() const
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





void NavMeshDebugRenderer::BuildDebugMesh(dtNavMesh* navMesh)
{
    if (!navMesh)
    {
        return;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    int maxTiles = navMesh->getMaxTiles();

    // 모든 타일 슬롯 순회
    for (int i = 0; i < maxTiles; ++i)
    {
        const dtMeshTile* tile = static_cast<const dtNavMesh*>(navMesh)->getTile(i);
        if (!tile || !tile->header)
            continue;

        // 이 타일의 모든 폴리곤 렌더링
        for (int j = 0; j < tile->header->polyCount; ++j)
        {
            const dtPoly* currentPoly = &tile->polys[j];

            // 폴리곤을 삼각형으로 분할 (Fan Triangulation)
            for (int k = 2; k < currentPoly->vertCount; ++k)
            {
                uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

                // 삼각형의 세 정점 추가
                const float* v0 = &tile->verts[currentPoly->verts[0] * 3];
                const float* v1 = &tile->verts[currentPoly->verts[k - 1] * 3];
                const float* v2 = &tile->verts[currentPoly->verts[k] * 3];

                vertices.push_back({
                    {v0[0], v0[1], v0[2]}
                    });

                vertices.push_back({
                    {v1[0], v1[1], v1[2]}
                    });

                vertices.push_back({
                    {v2[0], v2[1], v2[2]}
                    });

                // 인덱스 추가
                indices.push_back(baseIndex + 0);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);
            }
        }
    }

    mNavMeshSurfaceMesh = make_shared<Mesh>();
    mNavMeshSurfaceMesh->Init(vertices, indices);
}


void NavMeshDebugRenderer::DrawLine(const Vec3& start, const Vec3& end, const Vec4& color)
{
    RenderSystem::SubmitDebugLine(start, end, color);
}

void NavMeshDebugRenderer::RenderNavMesh(dtNavMesh* navMesh)
{
    if (!navMesh)
        return;

    // 흰색(NoDepth) — 색상 조건이 빨강/초록 어느 쪽도 아니면 mDebugLineNoDepthMat 선택
    // 깊이 테스트 없이 항상 렌더링 → 지형에 가려지지 않음
    const Vec4 walkableColor  = { 1.0f, 1.0f, 0.0f, 1.0f }; // 노란색 → NoDepth(흰색)으로 렌더링
    const Vec4 boundaryColor  = { 1.0f, 0.5f, 0.0f, 1.0f }; // 경계(외부 엣지) → NoDepth

    int maxTiles = navMesh->getMaxTiles();

    for (int i = 0; i < maxTiles; ++i)
    {
        const dtMeshTile* tile = static_cast<const dtNavMesh*>(navMesh)->getTile(i); // salt 무관하게 직접 접근
        if (!tile || !tile->header)
            continue;

        for (int j = 0; j < tile->header->polyCount; ++j)
        {
            const dtPoly* p = &tile->polys[j];

            // 오프메쉬 연결 폴리곤은 스킵
            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            // 폴리곤의 모든 엣지를 선분으로 렌더링
            for (int k = 0; k < p->vertCount; ++k)
            {
                const int next = (k + 1) % p->vertCount;

                const float* v0 = &tile->verts[p->verts[k]    * 3];
                const float* v1 = &tile->verts[p->verts[next] * 3];

                // 이웃 폴리곤이 없는 엣지(경계선)는 boundaryColor, 내부 엣지는 walkableColor
                const bool isBoundary = (p->neis[k] == 0);
                const Vec4& color = isBoundary ? boundaryColor : walkableColor;

                
                DrawLine(
                    Vec3(v0[2] * 100.f, v0[1] * 100.f, v0[0] * 100.f),
                    Vec3(v1[2] * 100.f, v1[1] * 100.f, v1[0] * 100.f),
                    color
                );
              /*  DrawLine(
                    Vec3(-v0[0] * 100.f, v0[1] * 100.f, v0[2] * 100.f),
                    Vec3(-v1[0] * 100.f, v1[1] * 100.f, v1[2] * 100.f),
                    color
                );*/
            }
        }
    }
}

void NavMeshDebugRenderer::RenderPath(const std::vector<Vec3>& path)
{
    // 경로 웨이포인트 사이를 빨간 선분으로 렌더링
    for (size_t i = 1; i < path.size(); ++i)
    {
        DrawLine(path[i - 1], path[i], { 1.0f, 0.0f, 0.0f, 1.0f });
    }
}
