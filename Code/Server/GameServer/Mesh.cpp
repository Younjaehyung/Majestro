#include "pch.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "FBX.h"

Mesh::Mesh() : Object(OBJECT_TYPE::MESH)
{
}

Mesh::~Mesh()
{
}

CollisionMesh::CollisionMesh() : Object(OBJECT_TYPE::COLLIDER)
{
}

CollisionMesh::~CollisionMesh()
{
}

void Mesh::CreateMesh(FBXBMeshInfo& f)
{
	CreateVertexBuffer(f.Vertices);
	for (const vector<uint32>& buffer : f.Indices)
	{
		if (buffer.empty())
		{
			vector<uint32> defaultBuffer{ 0 };
			CreateIndexBuffer(defaultBuffer);
		}
		else
		{
			CreateIndexBuffer(buffer);
		}
	}
}

void Mesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	mVertexBuffer = buffer;
}

void Mesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	mIndexBuffer = buffer;
}



void CollisionMesh::CreateMesh(FBXBMeshInfo& f)
{
	CreateVertexBuffer(f.Vertices);
	for (const vector<uint32>& buffer : f.Indices)
	{
		if (buffer.empty())
		{
			vector<uint32> defaultBuffer{ 0 };
			CreateIndexBuffer(defaultBuffer);
		}
		else
		{
			CreateIndexBuffer(buffer);
		}
	}
}

void CollisionMesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	mVertexBuffer = buffer;
}

void CollisionMesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	mIndexBuffer = buffer;
}

void CollisionMesh::CreateCollisionMesh(FBXBMeshInfo& f)
{
	CreateVertexBuffer(f.Vertices);

	if (mVertexBuffer.empty())
		return;

	// 모든 정점의 위치를 추출
	vector<Vec3> positions(mVertexBuffer.size());
	for (size_t i = 0; i < mVertexBuffer.size(); ++i)
	{
		positions[i] = mVertexBuffer[i].pos;
	}

	// 모든 점을 한 번에 전달하여 OBB 생성
	BoundingOrientedBox::CreateFromPoints(
		mOBB,
		positions.size(),
		positions.data(),
		sizeof(Vec3)
	);
}

void CollisionMesh::CreateCollisionMesh(vector<Vertex>& f)
{
	CreateVertexBuffer(f);

	if (mVertexBuffer.empty())
		return;

	// 모든 정점의 위치를 추출
	vector<Vec3> positions(mVertexBuffer.size());
	for (size_t i = 0; i < mVertexBuffer.size(); ++i)
	{
		positions[i] = mVertexBuffer[i].pos;
	}

	// 모든 점을 한 번에 전달하여 OBB 생성
	BoundingOrientedBox::CreateFromPoints(
		mOBB,
		positions.size(),
		positions.data(),
		sizeof(Vec3)
	);
}
