#pragma once
#include <vector>
#include "Object.h"
#include "LevelImport.h"
class Texture;
class RAW;
class Prefab;
class HeightField;
class Mesh;
class CollisionMesh;
class FBX;
class NavMesh;	

using KeyObjMap = std::map<wstring/*key*/, shared_ptr<Object>>;

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();
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


public:
	
	void LoadResources();
	LevelImportData LoadResourceJson(const std::wstring& path);
	shared_ptr<FBX>& LoadFBXMeshes(const wstring& path);

public:
	array<KeyObjMap, OBJECT_TYPE_COUNT> mResources;


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
	else if (std::is_same_v<T, Texture>)
		return OBJECT_TYPE::TEXTURE;
	else if (std::is_same_v<T, RAW>)
		return OBJECT_TYPE::RAW;
	else if (std::is_same_v<T, HeightField>)
		return OBJECT_TYPE::HEIGHTFIELD;
	else if (std::is_same_v<T, Mesh>)
		return OBJECT_TYPE::MESH;
	else if (std::is_same_v<T, NavMesh>)
		return OBJECT_TYPE::NAVMESH;
	else if (std::is_same_v<T, CollisionMesh>)
		return OBJECT_TYPE::COLLIDER;
	else if (std::is_same_v<T, FBX>)
		return OBJECT_TYPE::FBX;
	else
		return OBJECT_TYPE::NONE;
}