#pragma once
#include "Object.h"

struct BoneInfo
{
	string					boneName;
	int32					parentIdx;
	Matrix					matOffset;
	float					blendWeight = 1.f;
};

struct SkeletonBoneParams
{
	Matrix	matOffset;
	float	blendWeight = 1.f;
	float	padding[3]{};
};

class Skeleton : public Object
{
public:
	Skeleton();
	virtual ~Skeleton();
	virtual void Load(const wstring& path) {};
	
	void CreateBones(ifstream& file);

	std::vector<BoneInfo>& GetBones() { return mBones; }

	uint32 GetSkeletonHandle() { return mSkeletonHandle; }
	void SetSkeletonHandle(uint32 handle) { mSkeletonHandle = handle; }
public:
	uint32 mStartOffset{};
	uint32 mEndOffset{};
private:
	std::vector<BoneInfo>	mBones;
	uint32					mSkeletonHandle{};	// if Skeleton Enable use this Handle
public:
	friend class FBXData;
};

