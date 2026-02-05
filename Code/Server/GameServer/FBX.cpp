#include "pch.h"
#include "FBX.h"
#include "Mesh.h"
#include "GameCore.h"
#include "ResourceManager.h"
string ReadString(std::ifstream& file)
{
	uint32 length;
	file.read(reinterpret_cast<char*>(&length), sizeof(uint32));

	if (length == 0)
		return "";

	string utf8Str(length, '\0');
	file.read(&utf8Str[0], length);
	cout << utf8Str << endl;
	return utf8Str; // 기존의 s2ws 함수 사용
}


FBX::FBX() : Object(OBJECT_TYPE::FBX)
{

}

FBX::~FBX()
{
}

void FBX::Load(const wstring& path)
{
	mPath = ws2s(path);
	std::string filePath{ filesystem::path(mPath).parent_path().string() + "\\" + filesystem::path(mPath).filename().stem().string() };


	if (std::ifstream f(filePath + ".mesh", std::ios::binary); f) {
		f.read(reinterpret_cast<char*>(&mHeader), sizeof(mHeader));
		CreateMeshFromFBX(f);
	}
}

vector<shared_ptr<Mesh>>& FBX::CreateMeshFromFBX(ifstream& loader) {

	for (uint8 i = 0; i < mHeader.MeshCount; ++i) {
		shared_ptr<Mesh> mesh = make_shared<Mesh>();

		// === 1) .mesh ===

		string meshName = ReadString(loader);

		FBXMeshInfo metaMeshInfo{};
		loader.read(reinterpret_cast<char*>(&metaMeshInfo), sizeof(metaMeshInfo));

		FBXBMeshInfo meshInfo;

		// Mesh Load
		static_assert(std::is_trivially_copyable_v<Vertex>,
			"Vertex must be trivially copyable");
		meshInfo.Vertices.resize(metaMeshInfo.VertexCount);

		if (metaMeshInfo.VertexCount)
			loader.read(reinterpret_cast<char*>(meshInfo.Vertices.data()),
				sizeof(Vertex) * metaMeshInfo.VertexCount);

		// Indices (by material) Load
		meshInfo.Indices.resize(metaMeshInfo.MaterialCount);

		for (uint32 s = 0; s < metaMeshInfo.MaterialCount; ++s)
		{
			uint32 ic = 0;
			loader.read(reinterpret_cast<char*>(&ic), sizeof(ic));
			meshInfo.Indices[s].resize(ic);

			if (ic)
				loader.read(reinterpret_cast<char*>(meshInfo.Indices[s].data()),
					sizeof(uint32) * ic);
		}
		mesh->SetName(s2ws(meshName));
		mesh->CreateMesh(meshInfo);
		mMeshs.push_back(mesh);
		RESOURCEMANAGER.Add<Mesh>(mesh->GetName(), mesh);
	}
	loader.close();

	return mMeshs;
}