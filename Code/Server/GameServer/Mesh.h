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


private:
	vector<uint32>					mIndexBuffer;
	vector<Vertex>					mVertexBuffer;
	std::string mPath;
};

