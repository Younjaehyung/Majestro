#include "pch.h"
#include "Skeleton.h"
#include "FBXData.h"

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

uint32 Skeleton::FindBoneIndexByName(const string& boneName) const
{
	if (boneName.empty())
		return INVALID_BONE_INDEX;

	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		if (mBones[i].boneName == boneName)
			return i;
	}

	// 수정: FBX export 과정에서 대소문자만 달라지는 경우가 있어 trail socket 탐색은
	// 정확히 일치하지 않으면 한 번 더 대소문자를 무시하고 찾는다.
	string lowerTarget = boneName;
	std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		string lowerBone = mBones[i].boneName;
		std::transform(lowerBone.begin(), lowerBone.end(), lowerBone.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		if (lowerBone == lowerTarget)
			return i;
	}

	return INVALID_BONE_INDEX;
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

	
	constexpr bool kDebugAimBones = false;
	if constexpr (kDebugAimBones)
	{
		std::cout << "[AimBones] skel bones=" << mBones.size()
			<< " spine1=" << mSpine1Idx
			<< " spine2=" << mSpine2Idx
			<< " spine3=" << mSpine3Idx
			<< " neck=" << mNeckIdx << std::endl;
		for (uint32 i = 0; i < (uint32)mBones.size() && i < 20; ++i)
			std::cout << "  bone[" << i << "] = " << mBones[i].boneName << std::endl;
	}
}
