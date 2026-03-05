#pragma once

class NavMeshLoader
{
public:
    // Demo에서 저장한 .bin 파일 로드
    // 반환된 포인터는 사용 후 dtFreeNavMesh()로 해제
    static dtNavMesh* LoadFromDemoBin(const char* filePath);
};

class NavMeshDebugRenderer
{
public:
    struct DebugVertex
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT4 color;
    };

    // NavMesh에서 삼각형 버텍스 추출
    // 결과는 GetVertices()로 가져다가 DX12 버텍스버퍼에 올리면 됨
    void Extract(const dtNavMesh* navMesh);

    const std::vector<DebugVertex>& GetVertices() const { return m_Vertices; }
    bool IsEmpty() const { return m_Vertices.empty(); }

private:
    std::vector<DebugVertex> m_Vertices;
};
