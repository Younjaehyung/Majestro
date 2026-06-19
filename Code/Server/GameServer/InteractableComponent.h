#pragma once
#include "Component.h"
#include "GameEvents.h"


enum class InteractableKind : uint8
{
    // 즉시 회복. valueA = 회복량
    // 즉시 방어력 회복. valueA = 방어력량
    // 임펄스, valueA = 수직 임펄스량, valueB = 수평(XZ) 임펄스량
    // 이동속도 버프 buffType = BuffType
    // 겹치는 동안 데미지. vlaueA = 초당 데미지량, valueB = 지속 시간
    // 점령지. valueA = 점령지 번호(1부터), valueB = 점령에 필요한 시간(초, 0이면 기본값 사용)
    // 리스폰 지점

    None = 0,   
    HealPack, 
    ArmorPack,
	JumpPad,  
	SpeedPad, 
	DamageZone,
	ConquestZone,
	EscortZone,
    Checkpoint
};

enum class InteractableShape : uint8 { Box = 0, Sphere = 1 };

// 누구에게 적용되는지 필터
enum InteractableTarget : uint8
{   
    InteractableTarget_None   = 0,
    InteractableTarget_Player = 1 << 0,
    InteractableTarget_Enemy  = 1 << 1,
    InteractableTarget_All    = 0xFF,
};

class InteractableComponent : public Component<InteractableComponent>
{
public:
    InteractableComponent() = default;


    InteractableKind mKind = InteractableKind::None;
    float   mValueA = 0.0f;
    float   mValueB = 0.0f;
    SkillType mBuffType = SkillType::Default;

    // 어떤 태그를 가진 엔티티에 적용되는가
    uint8 mTargetMask = InteractableTarget_Player;

    float mCooldown = 0.0f;          // 0 이면 쿨타임X
	float mNextAvailableTime = 0.0f; // 다음 발동 가능 시간 (쿨다운 끝나는 시점)
    bool  mOneShot = false;          // 일회용 (먹힌 후 소멸)
    bool  mActive  = true;           // false면 스킵함
    bool  mHiddenForClients = false; // 클라이언트에 숨김 통지를 보낸 상태

    EntityID mLastUserId = 0;
    float    mLastTriggerTime = 0.0f;



    InteractableShape mShape = InteractableShape::Box; //기본 Box(기존 ConquestZone / HealPack 등 호환)
    float             mRadius = 0.f;                    //Sphere 일 때(cm)
    bool              mIgnoreY = true;                   // XZ 평면 거리만 비교
};
