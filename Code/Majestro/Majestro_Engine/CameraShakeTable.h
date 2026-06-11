#pragma once

enum PlayerType : uint8;

struct ShakePreset
{
	Vec3  mAngles    = Vec3::Zero; // pitch/yaw/roll 최대 흔들림 각도 (degree)
	float mDuration  = 0.f;        // 지속 시간 (초)
	float mFrequency = 20.f;       // 진동 주파수 (Hz)
};

class CameraShakeTable
{
public:
	static void Load(const std::string& path);

	// 등록된 프리셋이 없으면 nullptr (= 쉐이크 없음)
	static const ShakePreset* Find(PlayerType playerType, ReplicatedActionState state);
};
