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

bool Skeleton::SetUpperBodyEndByName(const string& boneName)
{
	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		if (mBones[i].boneName == boneName)
		{
			mSpineBoneEndOffset = i;
			return true;
		}
	}
	return false;
}

bool Skeleton::SetUpperBodyStartByName(const string& boneName)
{
	for (uint32 i = 0; i < static_cast<uint32>(mBones.size()); ++i)
	{
		if (mBones[i].boneName == boneName)
		{
			mSpineBoneStartOffset = i;
			return true;
		}
	}
	return false;
}
