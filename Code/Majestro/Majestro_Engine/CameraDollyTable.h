#pragma once


struct DollyPreset
{
	float mDistance = 100.f;  // mCameraMaxLenth에 더해질 추가 거리 (양수=뒤로, 음수=가까이)
	float mInSpeed  = 12.f;   // 빠질 때 보간 속도 (1/s, 클수록 빠르게 빠짐)
	float mOutSpeed = 3.f;    // 복귀할 때 보간 속도 (1/s, 작을수록 천천히 돌아옴)
};

class CameraDollyTable
{
public:
	static void Load(const std::string& path);

	// 등록된 프리셋이 없으면 nullptr (= dolly 없음)
	static const DollyPreset* Find(const std::string& presetName);
};
