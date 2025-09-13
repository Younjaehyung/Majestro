#include "pch.h"
#include "Animator.h"
#include "FBXData.h"

Animator::Animator() : Object(OBJECT_TYPE::ANIMATION)
{
}

Animator::Animator(FBXAnimClipInfo fbxData) : Object(OBJECT_TYPE::ANIMATION)
{
	mName = s2ws(fbxData.Name);
	mStartTime = fbxData.StartTime;
	mEndTime = fbxData.EndTime;
//	keyFrames = fbxData.KeyFrameInfo;

}

Animator::~Animator()
{
}

vector<Animator> Animator::CreateAnimations(ifstream& file)
{

	return vector<Animator>();
}
