#pragma once
#include "Object.h"

struct KeyFrameInfo
{
	double	time;
	int32	frame;
	Vec3	scale;
	Vec4	rotation;
	Vec3	translate;
};

struct AnimationClipMeta
{
	uint32 BoneCount;
	uint32 StartFrame;
	uint32 NumFrame;
	float FPS;
};


class Animator : public Object
{
public:
	Animator();
	Animator(struct FBXAnimClipInfo);
	virtual ~Animator();

	virtual void Load(const wstring& path) {};
	vector <Animator> CreateAnimations(ifstream& file);

	uint32			mSkeletonHandle{};	// if Skeleton Enable use this Handle

	int32			frameCount{};
	double			duration{};
	AnimationClipMeta				mClipMeta;
	vector<vector<KeyFrameInfo>>	keyFrames;

	double mStartTime{};
	double mEndTime{};

public:
	friend class FBXData;
};

