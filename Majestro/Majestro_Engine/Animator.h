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


class Animator : public Object
{
public:
	Animator();
	virtual ~Animator();

	virtual void Load(const wstring& path);
	vector <Animator> CreateAnimations(class FileLoader& loader);

	uint32			mSkeletonHandle;	// if Skeleton Enable use this Handle
	
	wstring			animName;
	int32			frameCount;
	double			duration;
	vector<vector<KeyFrameInfo>>	keyFrames;

};

