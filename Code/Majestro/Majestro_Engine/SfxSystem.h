#pragma once
#include "System.h"
#include "AudioManager.h"


struct SfxDesc
{
	std::string eventPath;      // "event:/SFX/..."
	bool  is3d = false;
	bool  loop = false;         // true 면 상태 연동 루프 (진입 시 시작, 이탈 시 페이드아웃 정지)
	float cooldown = 0.05f;     // 동일 키 재발행 최소 간격(초). 루프에는 미적용
};

class SfxSystem : public System
{
public:
	SfxSystem(World* world);

	void Initialize() override;
	void Update(float deltaTime) override;
	void OnWorldEnd() override;     // 보유한 루프 전부 정지


	void Play(const std::string& key, const Vec3* worldPos = nullptr);

private:
	// 엔티티당 상체/하체 루프 슬롯
	struct EntityLoopSlots
	{
		int upper = -1;             // 마지막으로 본 mUpperState
		int lower = -1;             // 마지막으로 본 mLowerState
		bool seen = false;
		SfxHandle upperLoop = 0;
		SfxHandle lowerLoop = 0;
	};


	struct EnemySfxState
	{
		int state = -1;
		bool seen = false;
		float nextFootstepTime = 0.f;
	};

	void LoadTable(const std::string& path);
	void ConsumeEvents();
	void PollPlayerStates();
	void PollEnemyStates();
	void StopAllOwnedLoops();

	// 미등록 키면 nullptr (최초 1회만 로그 찍게 둠)
	const SfxDesc* Find(const std::string& key);
	// 상태 변화 한 건 처리: 이전 루프 정지 후 새 키가 루프면 시작, 일회성이면 Play
	void HandleStateChange(SfxHandle& loopSlot, const std::string& key, const Vec3* pos);

	static FMOD_3D_ATTRIBUTES MakeAttr(const Vec3& position);

	// 키 조회
	static std::string SkillTypeName(SkillType type);
	static std::string SpawnReasonName(uint8 reason);
	static std::string PlayerTypeName(uint8 playerType);
	static std::string ActionStateName(int state);
	static std::string MovementModeName(int state);

	std::unordered_map<std::string, SfxDesc> mTable;
	std::unordered_map<std::string, float>   mLastPlayTime;   // key : 마지막 재생 시각
	std::unordered_map<EntityID, EntityLoopSlots> mSlots;
	std::unordered_map<EntityID, EnemySfxState> mEnemyStates;
	std::unordered_set<std::string> mWarnedKeys;              // 미등록/실패 키 1회 로그용
	float mDefaultCooldown = 0.05f;
	float mTime = 0.f;
};
