#pragma once
#include "Object.h"

class FBXBMeshInfo;

class Mesh : public Object
{
public:
	Mesh();
	~Mesh();

	void CreateMesh(FBXBMeshInfo& f);
	void CreateVertexBuffer(const vector<Vertex>& buffer);
	void CreateIndexBuffer(const vector<uint32>& buffer);

	void SetPath(const std::string& path) { mPath = path; }

	vector<Vertex>& GetVertexBuffer() { return mVertexBuffer; }
private:
	vector<uint32>					mIndexBuffer;
	vector<Vertex>					mVertexBuffer;
	std::string mPath;
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
	void CreateCollisionMesh(vector<Vertex>& f);

	void SetPath(const std::string& path) { mPath = path; }

	BoundingOrientedBox GetOBB() const { return mOBB; }
	// Jolt terrain raycast: expose CPU mesh data so PhysicsWorld can build a static triangle mesh.
	const vector<Vertex>& GetVertexBuffer() const { return mVertexBuffer; }
	const vector<uint32>& GetIndexBuffer() const { return mIndexBuffer; }
private:
	BoundingOrientedBox mOBB;

private:
	vector<uint32>					mIndexBuffer;
	vector<Vertex>					mVertexBuffer;
	std::string mPath;
};
