#pragma once
#include "Object.h"
struct SkeletonInfo 
{	
	uint32 StartOffset;
	uint32 EndOffset;
	vector<struct BoneWeight>					boneWeights;
};

struct BoneInfo
{
	string					boneName;
	int32					parentIdx;
	Matrix					matOffset;
};




class Skeleton : public Object
{
public:
	Skeleton();
	virtual ~Skeleton();
	virtual void Load(const wstring& path) {};
	void CreateBones(ifstream& file);
public:
	uint32 mStartOffset{};
	uint32 mEndOffset{};
private:
	std::vector<BoneInfo> mBones;

public:
	friend class FBXData;
};

