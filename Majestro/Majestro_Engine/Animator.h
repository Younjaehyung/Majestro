#pragma once
#include "Object.h"

class Skeleton;

struct KeyFrameInfo
{
	Vec4	scale;
	Vec4	rotation;
	Vec4	translate;


	// M: XMMATRIX (행렬은 아핀 변환이어야 함: 마지막 행/열이 [0 0 0 1])
	void DecomposeTRS(const Matrix& M, Vec4& outS, Vec4& outQ, Vec4& outT)
	{
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, M);

		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&outS), S);
		outS.w = 0.f; // 스케일 w 성분은 1로 설정
		XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&outQ), R); // (x,y,z,w) quaternion
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&outT), T);
		outT.w = 0.f; // 위치 w 성분은 1로 설정
	}

	KeyFrameInfo()
		: scale(1.f, 1.f, 1.f,0.f), rotation(0, 0, 0, 1.f), translate(0.f, 0.f, 0.f, 0.f)
	{
	}

	KeyFrameInfo(const Vec3& s, const Vec4& r, const Vec3& tr)
		: scale(s), rotation(r), translate(tr)
	{
	}

	KeyFrameInfo(struct FBXKeyFrameInfo& fbxKeyFrame);
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
	Animator(struct FBXAnimClipInfo&);
	virtual ~Animator();

	
	virtual void Load(const wstring& path) {};

	void SetSkeleton(shared_ptr<Skeleton> sk);
	shared_ptr<Skeleton> GetSkeleton() { return mSkeleton; }

	AnimationClipMeta& GetClipMeta() { return mClipMeta; }
	vector<vector<KeyFrameInfo>>& GetKeyFrames() { return mKeyFrames; }


public:
	double mStartTime{};
	double mEndTime{};

	shared_ptr<Skeleton> mSkeleton;


	int32			mFrameCount{};
	double			mDuration{};
	AnimationClipMeta				mClipMeta;
	vector<vector<KeyFrameInfo>>	mKeyFrames;


public:
	friend class FBXData;
};

