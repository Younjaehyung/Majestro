#pragma once
#include "Object.h"
struct SkeletonInfo 
{	
	uint32 StartOffset;
	uint32 EndOffset;
	
};

struct BoneInfo
{
	wstring					boneName;
	int32					parentIdx;
	Matrix					matOffset;
};

class Skeleton : public Object
{
public:
	Skeleton();
	virtual ~Skeleton();
	virtual void Load(const wstring& path);
	void CreateBones(class FileLoader& loader);
public:
	uint32 mStartOffset;
	uint32 mEndOffset;
private:
	std::vector<BoneInfo> mBones;

};

