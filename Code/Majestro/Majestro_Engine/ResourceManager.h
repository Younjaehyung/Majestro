#pragma once

#include "Prefab.h"
#include "Material.h"
#include "Mesh.h"
#include "Shader.h"
#include "Component.h"
#include "Texture.h"
#include "Animator.h"
#include "Skeleton.h"
#include "FBXData.h"
#include "Vfx.h"

using KeyObjMap = std::map<wstring/*key*/, shared_ptr<Object>>;

class ResourceManager
{

public:
	void Initialize();

	template<typename T>
	shared_ptr<T> Load(const wstring& key, const wstring& path);

	template<typename T>
	bool Add(const wstring& key, shared_ptr<T> object);

	template<typename T>
	shared_ptr<T> Get(const wstring& Key);

	template<typename T>
	OBJECT_TYPE GetObjectType();

	template<typename T>
	KeyObjMap& GetAllResources()
	{
		OBJECT_TYPE objectType = GetObjectType<T>();
		return mResources[static_cast<uint8>(objectType)];
	}

	

	shared_ptr<Mesh> LoadPointMesh();
	shared_ptr<Mesh> LoadRectangleMesh();
	shared_ptr<Mesh> LoadCubeMesh();
	shared_ptr<Mesh> LoadSphereMesh();
	shared_ptr<Mesh> LoadTerrainMesh(int32 sizeX, int32 sizeZ);

	shared_ptr<FBXData>		LoadFBX(const wstring& path);
	shared_ptr<Vfx>			LoadEffect(const wstring& path);
	void LoadAllTexture(const wstring& path);
	void LoadResourceJson(const wstring& path);


	//texture를 키로 매핑하기 위한 함수
	shared_ptr<Texture> CreateTexture(const wstring& name, DXGI_FORMAT format, uint32 width, uint32 height,
		const D3D12_HEAP_PROPERTIES& heapProperty, D3D12_HEAP_FLAGS heapFlags,
		D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE,bool createSRVUAV = 1, Vec4 clearColor = Vec4());


	shared_ptr<Texture> CreateTextureFromResource(const wstring& name, ComPtr<ID3D12Resource> tex2D, bool createSRVUAV = 1);


private:
	void CreateDefaultRootSignature();
	void CreateDefaultShader();	//기본적인 공통쉐이더를 공통적으로 적용하기 위한 코드
	void CreateDefaultMaterial();

private:
	
	array<KeyObjMap, OBJECT_TYPE_COUNT> mResources;

	// =>  리소스타입(총 OBJECT_TYPE_COUNT만큼)의 map이 존재.
};



template<typename T>
inline shared_ptr<T> ResourceManager::Load(const wstring& key, const wstring& path)
{
	OBJECT_TYPE objectType = GetObjectType<T>();
	KeyObjMap& keyObjMap = mResources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return static_pointer_cast<T>(findIt->second);
	//찾으면 출력

	shared_ptr<T> object = make_shared<T>();
	object->Load(path);
	keyObjMap[key] = object;
	//없으면 로딩

	return object;
}

template<typename T>	//파일이 아닌 직접 만든 객체 저장
bool ResourceManager::Add(const wstring& key, shared_ptr<T> object)
{
	OBJECT_TYPE objectType = GetObjectType<T>();
	KeyObjMap& keyObjMap = mResources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return false;

	keyObjMap[key] = object;

	return true;
}

template<typename T>
shared_ptr<T> ResourceManager::Get(const wstring& key)
{
	OBJECT_TYPE objectType = GetObjectType<T>();
	KeyObjMap& keyObjMap = mResources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return static_pointer_cast<T>(findIt->second);

	return nullptr;
}

template<typename T>
OBJECT_TYPE ResourceManager::GetObjectType()
{
	if (std::is_same_v<T, Prefab>)
		return OBJECT_TYPE::PREFAB;
	else if (std::is_same_v<T, Material>)
		return OBJECT_TYPE::MATERIAL;
	else if (std::is_same_v<T, Mesh>)
		return OBJECT_TYPE::MESH;
	else if (std::is_same_v<T, Animator>)
		return OBJECT_TYPE::ANIMATION;
	else if (std::is_same_v<T, Skeleton>)
		return OBJECT_TYPE::SKELETON;
	else if (std::is_same_v<T, FBXData>)
		return OBJECT_TYPE::FBXDATA;
	else if (std::is_same_v<T, Vfx>)
		return OBJECT_TYPE::VFX;
	else if (std::is_same_v<T, Shader>)
		return OBJECT_TYPE::SHADER;
	else if (std::is_same_v<T, Texture>)
		return OBJECT_TYPE::TEXTURE;
	else if (std::is_same_v<T, RootSignature>)
		return OBJECT_TYPE::ROOTSIGNATURE;
	else
		return OBJECT_TYPE::NONE;
}


