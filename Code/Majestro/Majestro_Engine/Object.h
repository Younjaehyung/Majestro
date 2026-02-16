#pragma once

enum class OBJECT_TYPE : uint8
{
	NONE,
	PREFAB, // PREFAB
	MATERIAL,
	MESH,
	COLLIDER,
	ANIMATION,
	SKELETON,
	FBXDATA,
	VFX,
	SHADER,
	TEXTURE,
	ROOTSIGNATURE,

	END
};

enum
{
	OBJECT_TYPE_COUNT = static_cast<uint8>(OBJECT_TYPE::END)
};

class Object
{
public:
	Object(OBJECT_TYPE type);
	virtual ~Object();
	OBJECT_TYPE GetObjectType() { return mObjectType; }

	void SetName(const wstring& name) { mName = name; }
	const wstring& GetName() { return mName; }

	uint32 GetID() { return mId; }


protected:
	friend class Resources;
	virtual void Load(const wstring& path) {}
	virtual void Save(const wstring& path) {}

protected:
	OBJECT_TYPE mObjectType = OBJECT_TYPE::NONE;
	wstring mName;

protected:
	uint32 mId = 0;
};