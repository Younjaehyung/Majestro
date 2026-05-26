#pragma once
#include "Component.h"


// 대쉬 잔상(afterimage) 한 개의 스냅샷.
struct DashTrailSample
{	// 본 팔레트는 저장하지 않고, 방출 시점에 캐릭터의 현재 포즈를 재사용.
	Matrix MatWorld{};   // 기록 당시 월드 행렬 (transpose 전 원본)
	float  Age = 0.f;    // 생성 후 경과 시간(초)
	bool   Alive = false;
};

// 대쉬 잔상 상태를 들고 다니는 컴포넌트.
class DashTrailComponent : public Component<DashTrailComponent>
{
public:
	static constexpr uint32 MAX_SAMPLES = 8;

	std::array<DashTrailSample, MAX_SAMPLES> mSamples{};
	uint32 mHead = 0;            // 다음 기록 슬롯 (링 버퍼)
	float  mSpawnTimer = 0.f;    // 샘플 스폰 누적 타이머
	bool   mWasDashing = false;  // 직전 프레임 대쉬 여부 (대쉬 시작 즉시 첫 샘플 생성용)

	// 설정값
	float mSpawnInterval = 0.028f; // 샘플 간 시간 간격(초) — 작을수록 잔상이 촘촘
	float mLifetime      = 0.30f;  // 샘플 수명(초) — 길수록 꼬리가 김
	float mBaseAlpha     = 0.55f;  // 가장 최신 샘플의 시작 알파(0~1)
};
