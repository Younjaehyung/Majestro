#include "pch.h"
#include "Animator.h"
#include "FBXData.h"
#include "Skeleton.h"

Animator::Animator() : Object(OBJECT_TYPE::ANIMATION)
{
}

Animator::Animator(FBXAnimClipInfo& fbxData) : Object(OBJECT_TYPE::ANIMATION)
{
	mName = s2ws(fbxData.Name);
	mStartTime = fbxData.StartTime;
	mEndTime = fbxData.EndTime;
	mDuration = mEndTime - mStartTime;

	mClipMeta.StartFrame = 0;
	mClipMeta.FPS = 30.0f; // to-do
	mFrameCount = static_cast<int>((mEndTime - mStartTime) * mClipMeta.FPS + 0.5f);

	
	mClipMeta.NumFrame = mFrameCount;
	
	
	mKeyFrames.resize(fbxData.KeyFrameInfo.size());

	for(uint32 bi = 0; bi < fbxData.KeyFrameInfo.size(); ++bi)
	{

		mKeyFrames[bi].reserve(fbxData.KeyFrameInfo[bi].size());
		for (uint32 ki = 0; ki < fbxData.KeyFrameInfo[bi].size(); ++ki)
		{
			mKeyFrames[bi].emplace_back(KeyFrameInfo(fbxData.KeyFrameInfo[bi][ki]));
		}
	}
}

Animator::~Animator()
{
}

void Animator::SetSkeleton(shared_ptr<Skeleton> sk)
{
	mSkeleton = sk;
	mClipMeta.BoneCount = mSkeleton->GetBones().size(); 
}

KeyFrameInfo::KeyFrameInfo(FBXKeyFrameInfo& fbxKeyFrame)
	{
		Matrix m = Matrix(fbxKeyFrame.MatTransform);
		DecomposeTRS(m, scale, rotation, translate);
	}