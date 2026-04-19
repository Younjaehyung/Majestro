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
	int32	parentIdx = -1;
	float	padding[2]{};
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

	// AimOffset 용 spine 체인 본 인덱스 캐시(보유하지 않으면 UINT32_MAX)
	void BuildAimBoneIndices();

public:
	uint32 mStartOffset{};
	uint32 mEndOffset{};

	uint32 mSpineBoneCount{};
	uint32 mSpineBoneStartOffset{};
	uint32 mSpineBoneEndOffset{};

	static constexpr uint32 INVALID_BONE_INDEX = UINT32_MAX;
	uint32 mSpine1Idx = INVALID_BONE_INDEX;
	uint32 mSpine2Idx = INVALID_BONE_INDEX;
	uint32 mSpine3Idx = INVALID_BONE_INDEX;
	uint32 mNeckIdx   = INVALID_BONE_INDEX;
private:
	std::vector<BoneInfo>	mBones;
	uint32					mSkeletonHandle{};	// if Skeleton Enable use this Handle
public:
	friend class FBXData;
};

