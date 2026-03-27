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

	// 이름으로 Upper 마스크 끝 인덱스 설정
	// FBX 로딩 완료 후 씬/프리팹 초기화 코드에서 직접 호출한다.
	// 반환값: 해당 이름의 본을 찾으면 true, 없으면 false (기존 자동 탐지값 유지)
	bool SetUpperBodyEndByName(const string& boneName);

	// 이름으로 Upper 마스크 시작 인덱스 설정 (Spine 시작점 수동 지정 시 사용)
	bool SetUpperBodyStartByName(const string& boneName);
public:
	uint32 mStartOffset{};		// AnimationSystem 초기화 시 GPU 버퍼 내 본 시작 오프셋
	uint32 mEndOffset{};

	// ─── 상하체 분리 마스크 정보 (FBXData 로딩 시 자동 계산) ───────────────
	// mSpineBoneCount      : 스켈레톤 전체 본 개수
	// mSpineBoneStartOffset: Upper 마스크 시작 인덱스
	//                        blendWeight > 0 인 첫 번째 본 (spine 전환 구간 시작)
	// mSpineBoneEndOffset  : Upper 마스크 끝 인덱스 (마지막 본)
	uint32 mBoneCount{};
	uint32 mSpineBoneStartOffset{};
	uint32 mSpineBoneEndOffset{};
private:
	std::vector<BoneInfo>	mBones;
	uint32					mSkeletonHandle{};	// if Skeleton Enable use this Handle
public:
	friend class FBXData;
};

