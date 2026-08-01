#include "pch.h"
#include "Skeleton.h"
#include "FBXData.h"
#include "EngineLog.h"

Skeleton::Skeleton() : Object(OBJECT_TYPE::SKELETON)
{
}

Skeleton::~Skeleton()
{
}


void Skeleton::CreateBones(ifstream& file)
{

	// === 2) .skel ===



}

void Skeleton::BuildAimBoneIndices()
{
	mSpine1Idx = INVALID_BONE_INDEX;
	mSpine2Idx = INVALID_BONE_INDEX;
	mSpine3Idx = INVALID_BONE_INDEX;
	mNeckIdx   = INVALID_BONE_INDEX;

	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		string lower = mBones[i].boneName;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		
		if (mSpine1Idx == INVALID_BONE_INDEX && lower.find("spine1") != string::npos)
			mSpine1Idx = i;
		else if (mSpine2Idx == INVALID_BONE_INDEX && lower.find("spine2") != string::npos)
			mSpine2Idx = i;
		else if (mSpine3Idx == INVALID_BONE_INDEX && lower.find("spine3") != string::npos)
			mSpine3Idx = i;
		else if (mNeckIdx == INVALID_BONE_INDEX && lower.find("neck") != string::npos)
			mNeckIdx = i;
	}


	if (mSpine1Idx == INVALID_BONE_INDEX &&
		mSpine2Idx == INVALID_BONE_INDEX &&
		mSpine3Idx == INVALID_BONE_INDEX &&
		mNeckIdx == INVALID_BONE_INDEX)
	{
		for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
		{
			string lower = mBones[i].boneName;
			std::transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (lower.find("spine") != string::npos || lower.find("chest") != string::npos)
			{
				mSpine1Idx = i;
				break;
			}
		}
	}

	

	if constexpr (EngineLog::Enabled(EngineLog::Domain::SkeletonBones))
	{
		EngineLog::WriteTagged(EngineLog::Domain::SkeletonBones, "aim-bones",
			"bones=", mBones.size(),
			" spine1=", mSpine1Idx, " spine2=", mSpine2Idx,
			" spine3=", mSpine3Idx, " neck=", mNeckIdx);

		for (uint32 i = 0; i < (uint32)mBones.size() && i < 20; ++i)
			EngineLog::WriteTagged(EngineLog::Domain::SkeletonBones, "aim-bones",
				"bone[", i, "] = ", mBones[i].boneName);
	}
}

void Skeleton::BuildUpperBodyMaskRange()
{
	mSpineBoneStartOffset = 0;
	mSpineBoneEndOffset = 0;
	mSpineBoneCount = 0;

	const uint32 boneCount = static_cast<uint32>(mBones.size());
	if (boneCount == 0)
		return;

	// 상체 레이어 마스크 범위는 고정 인덱스가 아니라 본 이름으로 자동 계산한다.
	// 시작 본은 pelvis와 다리 본을 건너뛰어 공격과 조준 클립이 하체 이동을 덮어쓰지 않게 한다.
	uint32 upperStart = INVALID_BONE_INDEX;
	for (uint32 i = 0; i < boneCount; ++i)
	{
		const string lower = ToLowerBoneName(mBones[i].boneName);
		if (ContainsAnyBoneToken(lower, { "spine1", "spine_01", "spine01", "chest", "upperchest","pelvis"}))
		{
			upperStart = i;
			break;
		}
	}

	if (upperStart == INVALID_BONE_INDEX)
	{
		for (uint32 i = 0; i < boneCount; ++i)
		{
			const string lower = ToLowerBoneName(mBones[i].boneName);
			if (ContainsAnyBoneToken(lower, { "spine2", "spine_02", "spine02", "spine" }))
			{
				upperStart = i;
				break;
			}
		}
	}

	if (upperStart == INVALID_BONE_INDEX)
	{
		for (uint32 i = 0; i < boneCount; ++i)
		{
			if (mBones[i].blendWeight >= 0.7f)
			{
				upperStart = i;
				break;
			}
		}
	}

	uint32 upperEnd = INVALID_BONE_INDEX;
	for (uint32 i = 0; i < boneCount; ++i)
	{
		const string upper = ToLowerBoneName(mBones[i].boneName);
		if (ContainsAnyBoneToken(upper, { "head", "headnup" }))
		{
			upperEnd = i;
			break;
		}
	}

	if (upperStart == INVALID_BONE_INDEX)
		upperStart = 0;

	if (upperEnd == INVALID_BONE_INDEX)
		upperEnd = boneCount - 1;

	mSpineBoneStartOffset = upperStart;
	mSpineBoneEndOffset = upperEnd;
	mSpineBoneCount = mSpineBoneEndOffset - mSpineBoneStartOffset + 1;
}

void Skeleton::BuildBoneNameIndex()
{
	mBoneNameToIndex.clear();
	mBoneNameToIndexLower.clear();
	mBoneNameToIndex.reserve(mBones.size());
	mBoneNameToIndexLower.reserve(mBones.size());

	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		const string& name = mBones[i].boneName;
		mBoneNameToIndex.emplace(name, i);
		mBoneNameToIndexLower.emplace(ToLowerBoneName(name), i);
	}
}

bool Skeleton::TryFindBoneIndex(const string& boneName, uint32& outIndex) const
{
	// 정확 일치 우선 검색
	if (auto it = mBoneNameToIndex.find(boneName); it != mBoneNameToIndex.end())
	{
		outIndex = it->second;
		return true;
	}

	
	string lower = boneName;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// 대소문자 무시 검색
	if (auto it = mBoneNameToIndexLower.find(lower); it != mBoneNameToIndexLower.end())
	{
		outIndex = it->second;
		return true;
	}

	return false;
}

string Skeleton::ToLowerBoneName(const string& boneName)
{
	string lower = boneName;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return lower;
}

bool Skeleton::ContainsAnyBoneToken(const string& lower, const std::initializer_list<const char*> tokens)
{
	for (const char* token : tokens)
	{
		if (lower.find(token) != string::npos)
			return true;
	}

	return false;
}