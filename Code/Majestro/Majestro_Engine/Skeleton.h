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
	void BuildUpperBodyMaskRange();

	// 본 이름 -> 인덱스 매핑
	void BuildBoneNameIndex();
	// Bonename -> index 검색
	bool TryFindBoneIndex(const string& boneName, uint32& outIndex) const;
private:
	string ToLowerBoneName(const string& boneName);
	bool   ContainsAnyBoneToken(const string& lower, const std::initializer_list<const char*> tokens);

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

	// WeaponTrail(Socket) change: 본 이름 -> 인덱스 조회 캐시(BuildBoneNameIndex에서 채움)
	std::unordered_map<string, uint32>	mBoneNameToIndex;		// 정확 일치(원본 이름)
	std::unordered_map<string, uint32>	mBoneNameToIndexLower;	// 소문자(대소문자 무시 fallback)
public:
	friend class FBXData;
};

