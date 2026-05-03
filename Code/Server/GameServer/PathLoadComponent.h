#pragma once
#include "Component.h"
#include "PayloadPathData.h"


class PathLoadComponent : public Component<PathLoadComponent>
{
	public:
	PathLoadComponent() = default;
	PathLoadComponent(const std::wstring& path);

	Vec3 mBaseOffset = Vec3::Zero;

	std::wstring mPath;
	shared_ptr<PayloadPathData> mPathData;

	float mCurrentDistance  = 0.f; // 현재 경로 상의 위치 (거리, cm)
	float mPreviousDistance = 0.f; // 이전 프레임 거리 — 이벤트 통과 판정용
	float mTotalDistance = 0.f;		 // 경로 전체 길이 (cm)
	float mBaseSpeed        = 0.f; // 기본 이동 속도 (cm/s)

	bool  mActive    = true;       // false 면 PathFollowSystem 이 무시
	bool  mPaused    = true;      // StopPoint 대기 중인지

	std::unordered_set<std::string> mFiredEvents; // fireOnce 이벤트 중복 방지
};
