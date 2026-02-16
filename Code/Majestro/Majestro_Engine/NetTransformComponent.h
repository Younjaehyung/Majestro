#pragma once
#include "Component.h"

struct NetSnapshot
{
	uint32 serverTick;
	Vec3     pos;
	Vec3     vel;
	Quaternion     rotQ; // Euler 또는 Quat
};


class NetTransformComponent : public Component<NetTransformComponent>
{
public:
	Vec3 mStartPosition{};
	Vec3 mTargetPosition{};
	Vec3 mStartRotation{};
	Vec3 mTargetRotation{};
	Quaternion mTargetRotationQ{};

	Vec3 mVelocity{};

	float mElapsed = 0.0f;
	float mDuration = 0.1f;
	bool mHasTarget = false;
	bool mInitialized = false;

	float	mLastUpdateTime = 0.0f;
	uint32 mLastSequence = 0;
public:
	std::vector<NetSnapshot> mBuffer;

	Vec3 mRenderPos{};
	Quaternion mRenderRotQ{};
	Vec3 mRenderRotE{};

	double mServerHz = 60.0;
	double mFixedDtSec = 1.0 / 60.0;

	// [추가] 스냅샷 주기 관련
	double mSnapshotHz = 20.0;

	// [변경] 초 기반이 아니라 tick 기반으로 들고 있음
	double mInterpDelayTicksF = 7.5;  // 예: 60Hz에서 0.125s면 7.5 ticks
	double mMaxExtrapolationTicksF = 9.0;  // 예: 0.15s * 60Hz = 9 ticks
	double mMaxExtrapolationSec = 0.15; // 유지(튜닝 편의)

	// [추가] 서버-now tick 추정용 앵커
		bool    mHasAnchor = false;
	uint32_t mAnchorServerTick = 0;
	double  mAnchorLocalSec = 0.0;

	// [추가] (선택) RTT/2 등을 반영한 lead tick
	double mLatencyLeadTicksF = 0.0;
public:
	void OnSnapshot(const NetSnapshot& s, double localRecvSec);

	void UpdateRender(double localNowSec);

	void SetServerHz(double serverHz);

	void SetSnapshotRateHz(double snapshotHz);

	void SetLatencyLeadTicks(double leadTicks);

	static bool IsNewer(uint32_t a, uint32_t b);

	static Vec3 Hermite(const Vec3& p0, const Vec3& m0, const Vec3& p1, const Vec3& m1, float t);

	void InsertSorted(const NetSnapshot& s);

	bool FindBracketing(double renderTickF, NetSnapshot& outA, NetSnapshot& outB) const;

	void PruneOld(double keepSec);

	void RecomputeTickParams();

	Vec3 GetRenderPos() const { return mRenderPos; }
	Quaternion GetRenderRotQ() const { return mRenderRotQ; }
	Vec3 GetRenderRotE() const { return mRenderRotE; }
};