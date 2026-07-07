#pragma once
#include "Component.h"
#include "Entity.h"

// 광장 NPC 역할
enum class NpcRole : uint8
{
	Dialogue = 0,   // 대화만
	LevelSelect,    // 레벨 진입 게이트 (2단계에서 선택지 연동)
	TraitSelect,    // 특성 선택 (로드아웃 작업과 함께)
	RhythmSelect,   // 리듬(음악) 선택
};

class NpcComponent : public Component<NpcComponent>
{
public:
	NpcComponent() = default;

	std::wstring mNpcId;
	std::wstring mName;                        // 대화창 표시 이름
	std::wstring mPortraitKey = L"UI_NoteMan_Portrait";  // 대화창 초상화 텍스처 키
	NpcRole mRole = NpcRole::Dialogue;
	float mInteractRadius = 250.f;
	float mHomeYaw = 0.f;                      // 배치 방향 (대화 종료 후 복귀할 yaw, 도 단위)
	std::vector<std::wstring> mDialogueLines;  // 1단계: 선형 대사
};

// 대화 진행 상태 (월드 싱글톤)
class DialogueStateComponent : public Component<DialogueStateComponent>
{
public:
	DialogueStateComponent() = default;

	bool mActive = false;      // 대화창 열림
	Entity mNpc{};             // 대화 중인 NPC
	int32 mLineIndex = 0;      // 현재 대사 인덱스

	Entity mNearbyNpc{};       // 상호작용 반경 안의 NPC (프롬프트 표시용)

	// 레벨 선택 UI (관문지기 — NpcRole::LevelSelect)
	bool mLevelSelectActive = false;  // 레벨 선택 UI 열림
	int32 mSelectedStage = 0;         // 선택된 스테이지 (0=First, 1=Second, 2=Third)
};
