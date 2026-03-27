#pragma once
#include "Object.h"

class Skeleton;

struct KeyFrameInfo
{
	Vec4	scale{ 1,1,1,1 };
	Vec4	rotation{ 0,0,0,1 };
	Vec4	translate{ 0,0,0,1 };



	bool DecomposeTRS(const Matrix& M, Vec4& outS, Vec4& outQ, Vec4& outT)
	{
		XMVECTOR S, R, T;
		const bool ok = XMMatrixDecompose(&S, &R, &T, M);

		if (!ok) {
			outS = { 1,1,1,1 };
			outQ = { 0,0,0,1 };
			outT = { 0,0,0,1 };
			return false;
		}

		outS = S;
		outS.w = 1.f;

		outQ = R; // quaternion(x,y,z,w)

		outT = T;
		outT.w = 1.f;

		return true;
	}


	KeyFrameInfo()
		: scale(1.f, 1.f, 1.f,1.f), rotation(0, 0, 0, 1.f), translate(0.f, 0.f, 0.f, 1.f)
	{
	}

	KeyFrameInfo(const Vec3& s, const Vec4& r, const Vec3& tr)
		: scale(s), rotation(r), translate(tr)
	{
		scale.w = 1.f;
		translate.w = 1.f;
	}

	KeyFrameInfo(struct FBXKeyFrameInfo& fbxKeyFrame);
};

struct AnimationClipMeta
{
	uint32 BoneStart{};
	uint32 BoneCount{};

	uint32 StartFrame{};
	uint32 NumFrame{};
	uint32 AnimOffset{}; // KeyFrameInfo Offset in AnimationClip Buffer
};


class Animator : public Object	// Animation Clip
{
public:
	Animator();
	Animator(struct FBXAnimClipInfo&);
	virtual ~Animator();

	
	virtual void Load(const wstring& path) {};

	void SetSkeleton(shared_ptr<Skeleton> sk);
	shared_ptr<Skeleton>& GetSkeleton() { return mSkeleton; }

	void SetAnimClipOffset(uint32 handle) { mClipMeta.AnimOffset = handle; }
	uint32 GetAnimClipOffset() { return  mClipMeta.AnimOffset; }

	void SetAnimClipHandle(uint32 handle) { mAnimClipHandle = handle; }
	uint32 GetAnimClipHandle() { return  mAnimClipHandle; }

	void SetAnimSkelOffset(uint32 handle) { mClipMeta.BoneStart = handle; }
	uint32 GetAnimSkelOffset() { return  mClipMeta.BoneStart; }

	void SetClipMeta(const AnimationClipMeta& meta) { mClipMeta = meta; }
	AnimationClipMeta& GetClipMeta() { return mClipMeta; }
	vector<vector<KeyFrameInfo>>& GetKeyFrames() { return mKeyFrames; }

public:
	double							mStartTime{};
	double							mEndTime{};
	double							mDuration{};

	AnimationClipMeta				mClipMeta{};		// Meta 데이터
	
private:
	uint32							mAnimClipHandle{};	// 현재 몇번째 메타 데이터인지
	shared_ptr<Skeleton>			mSkeleton;
	vector<vector<KeyFrameInfo>>	mKeyFrames;
public:
	friend class FBXData;
};

