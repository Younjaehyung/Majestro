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
