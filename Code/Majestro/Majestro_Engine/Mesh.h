#pragma once
#include "Object.h"

class Material;
class FBXData;

struct IndexBufferInfo
{
	ComPtr<ID3D12Resource>		Buffer;
	D3D12_INDEX_BUFFER_VIEW		BufferView;
	DXGI_FORMAT					Format;
	uint32						Count;
};

// [유니티짱]과 같이 정점으로 이루어진 물체
class Mesh : public Object
{
public:

	Mesh();
	virtual ~Mesh();

	void Init(const vector<Vertex>& vec, const vector<uint32>& indexbuffer);
	void Render(uint32 instanceCount = 1, uint32 idx = 0, uint32 baseInstance = 0, uint32 instancingID = 0);
	//void Render(uint8 index,uint32 instanceCount = 1, uint32 idx = 0);
	void Render(shared_ptr<class InstancingBuffer>& buffer, uint32 idx = 0);

	virtual void Load(const wstring& path);
	void CreateMesh(struct FBXBMeshInfo& f);
	void CreateVertexBuffer(const vector<Vertex>& buffer);
	void CreateIndexBuffer(const vector<uint32>& buffer);
private:
	ComPtr<ID3D12Resource>		mVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	mVertexBufferView = {};
	uint32 mVertexCount = 0;

	vector<IndexBufferInfo>		mVecIndexInfo;

	//bufferView : buffer의 주소값과 정보들이 담겨있음
	
	uint32						mSkeletonHandle;	// if Skeleton Enable use this Handle

public:
	friend class FBXData;
};


class CollisionMesh : public Object
{
public:
	CollisionMesh();
	~CollisionMesh();

	void CreateMesh(FBXBMeshInfo& f);
	void CreateVertexBuffer(const vector<Vertex>& buffer);
	void CreateIndexBuffer(const vector<uint32>& buffer);

	void CreateCollisionMesh(FBXBMeshInfo& f);

	void SetPath(const std::string& path) { mPath = path; }

	BoundingOrientedBox GetOBB() const { return mOBB; }
private:
	BoundingOrientedBox mOBB;

private:
	vector<uint32>					mIndexBuffer;
	vector<Vertex>					mVertexBuffer;
	std::string mPath;
};