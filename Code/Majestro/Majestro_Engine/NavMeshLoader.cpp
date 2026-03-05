#include "pch.h"
#include "NavMeshLoader.h"


// Demo 저장 포맷 매직넘버 (Sample_SoloMesh.cpp 기준)
static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T';
static const int NAVMESHSET_VERSION = 1;

// Demo bin 파일 헤더 구조 (Demo 소스와 정확히 일치해야 함)
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

dtNavMesh* NavMeshLoader::LoadFromDemoBin(const char* filePath)
{
    FILE* fp = nullptr;
    fopen_s(&fp, filePath, "rb");
    if (!fp) return nullptr;

    // 헤더 읽기
    NavMeshSetHeader header{};
    fread(&header, sizeof(NavMeshSetHeader), 1, fp);

    // 유효성 검사
    if (header.magic != NAVMESHSET_MAGIC)
    {
        fclose(fp);
        return nullptr; // 포맷 불일치
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        fclose(fp);
        return nullptr; // 버전 불일치
    }

    // NavMesh 초기화
    dtNavMesh* navMesh = dtAllocNavMesh();
    if (!navMesh)
    {
        fclose(fp);
        return nullptr;
    }

    dtStatus status = navMesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        dtFreeNavMesh(navMesh);
        fclose(fp);
        return nullptr;
    }

    // 타일 데이터 읽기
    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader{};
        fread(&tileHeader, sizeof(NavMeshTileHeader), 1, fp);

        if (!tileHeader.tileRef || !tileHeader.dataSize)
            continue;

        // Detour 전용 할당자로 메모리 확보 (중요: 일반 new/malloc 사용 금지)
        uint8_t* data = (uint8_t*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data) break;

        memset(data, 0, tileHeader.dataSize);
        fread(data, tileHeader.dataSize, 1, fp);

        // DT_TILE_FREE_DATA: NavMesh가 메모리 해제 책임을 가져감
        navMesh->addTile(data, tileHeader.dataSize,
            DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
    }

    fclose(fp);
    return navMesh;
}


void NavMeshDebugRenderer::Extract(const dtNavMesh* navMesh)
{
    m_Vertices.clear();

    if (!navMesh) return;

    for (int i = 0; i < navMesh->getMaxTiles(); ++i)
    {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (!tile || !tile->header) continue;

        for (int j = 0; j < tile->header->polyCount; ++j)
        {
            const dtPoly* poly = &tile->polys[j];
            const dtPolyDetail* detail = &tile->detailMeshes[j];

            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

            for (int k = 0; k < detail->triCount; ++k)
            {
                const uint8_t* tri =
                    &tile->detailTris[(detail->triBase + k) * 4];

                // 삼각형 3개 버텍스 추출
                for (int v = 0; v < 3; ++v)
                {
                    const float* src = nullptr;

                    if (tri[v] < poly->vertCount)
                        src = &tile->verts[poly->verts[tri[v]] * 3];
                    else
                        src = &tile->detailVerts[
                            (detail->vertBase + tri[v] - poly->vertCount) * 3];

                    DebugVertex vert{};

                    // Recast Y-up → DX12 프레임워크 좌표계로 변환
                    // 프레임워크가 Y-up이면 그대로, Z-up이면 아래 변환 적용
                    vert.pos = { src[0], src[1], src[2] };  // Y-up 그대로
                    // vert.pos = { src[0], src[2], src[1] }; // Z-up이면 이걸로

                    vert.color = { 0.0f, 0.8f, 0.2f, 0.4f }; // 반투명 초록

                    m_Vertices.push_back(vert);
                }
            }
        }
    }
}