#pragma once
#include "Entity.h"
#include "Component.h"

class Mesh;
class Material;

// Gameplay와 Pause 및 결과 화면의 UI를 렌더 단계에서 분리하기 위한 그룹이다.
enum class UIRenderGroup : uint8
{
	Gameplay = 0,
	Pause,
	Clear,
	GameOver
};

// 현재 화면 상태에 따라 렌더링할 UI를 구분한다.
// 별도 컴포넌트가 없는 기존 UI는 Gameplay 그룹으로 처리한다.
class UIRenderGroupComponent : public Component<UIRenderGroupComponent>
{
public:
	UIRenderGroupComponent() = default;
	explicit UIRenderGroupComponent(UIRenderGroup group)
		: mGroup(group)
	{
	}

	UIRenderGroup mGroup = UIRenderGroup::Gameplay;
};

// UIScriptComponent.h
class UIScriptComponent : public Component<UIScriptComponent>
{
public:
	std::function<void(float dt)> mOnUpdate;  // 매 프레임 호출
	std::function<void()>         mOnInit;    // 최초 1회
};

struct UIHpLossTriangle // 잃은 HP 공간을 채우는 파편 삼각형 하나.
{
	// 좌표는 앵커 기준 화면 픽셀 오프셋
	Vec2 V0 = Vec2::Zero;
	Vec2 V1 = Vec2::Zero;
	Vec2 V2 = Vec2::Zero;
};

struct UIHpLossFragment
{
	// 한 번의 피해 이벤트에서 생성된 파편 집합.
	// 오프셋은 생성 시점에 고정되고 수명 동안 알파만 감소.
	std::vector<UIHpLossTriangle> Triangles;

	// Hit effect(스프라이트 시트 오버레이) 앵커 — 피격 순간의 남은 HP 우측 끝
	Vec2 HitAnchorPx = Vec2::Zero;
	// 이번 피해로 잃은 바 폭(픽셀). hit 스프라이트 가로(U) 크기를 피해량만큼 늘리는 데 사용.
	float LostWidthPx = 0.f;
	float Age = 0.f;
	float LifeTime = 0.35f;
};

// 채움 비율(0~1)을 텍스처 캔버스 UV 위치로 자르기
inline float RemapBarRatioToUv(float ratio, const Vec2& uvRange)
{
	return uvRange.x + ratio * (uvRange.y - uvRange.x);
}

class UIHpBarComponent : public Component<UIHpBarComponent>
{
public:
	UIHpBarComponent() = default;
	UIHpBarComponent(
		float maxWidth,
		Entity targetEntity = NULL_ENTITY,
		const Vec3& worldOffset = Vec3(0.f, 120.f, 0.f),
		float height = 20.f,
		const std::wstring& backgroundMaterialName = L"UI_Player_HP_1",
		const std::wstring& fillMaterialName = L"UI_Player_HP_2")
		: mMaxWidth((std::max)(0.0f, maxWidth))
		, mTargetEntity(targetEntity)
		, mWorldOffset(worldOffset)
		, mHeight((std::max)(0.0f, height))
		, mBackgroundMaterialName(backgroundMaterialName)
		, mFillMaterialName(fillMaterialName.empty() ? backgroundMaterialName : fillMaterialName) {
	}
public:
	float mMaxWidth = 180.0f;
	float mHeight = 80.0f;
	Entity mTargetEntity = NULL_ENTITY;
	Vec3 mWorldOffset = Vec3(0.f, 120.f, 0.f);
	Vec2 mScreenOffsetPx = Vec2(0.f, 0.f);
	std::wstring mBackgroundMaterialName = L"UI_Player_HP_1";
	std::wstring mFillMaterialName = L"UI_Player_HP_2";

	// 채움 텍스처 캔버스 안에서 실제 바 그림이 차지하는 UV 구간
	Vec2 mFillUvRangeX = Vec2(0.f, 1.f);
	Vec2 mFillUvRangeY = Vec2(0.f, 1.f);

	// 쉴드(아머) 바 — 비어있으면 미표시. (적 월드 바 전용; mRenderBgFill=true 인 셰이더 경로)
	// HP 채움 아래에 그린 뒤 HP가 위를 덮어, "현재 체력 오른쪽"부터 (curHp+shield)/denom 까지만 보인다.
	// 쉴드 텍스처의 색 영역(UV)을 HP 바 하우징에 리맵해 같은 줄에 정렬한다.
	std::wstring mShieldMaterialName;
	Vec2 mShieldUvRangeX = Vec2(0.f, 1.f);
	Vec2 mShieldUvRangeY = Vec2(0.f, 1.f);

	Entity mBackgroundUIEntity = NULL_ENTITY;
	Entity mFillUIEntity = NULL_ENTITY;
	bool mHasPreviousHpRatio = false;
	float mPreviousHpRatio = 1.f;
	std::vector<UIHpLossFragment> mLossFragments;
	float mFragmentLifeTime = .35f;
	int mMaxFragmentCount = 24;

	// HUD 모드: 월드 좌표 무시, 
	// UITransformComponent::mFinalPixelPos를 화면 픽셀 앵커로 사용.
	bool mIsScreenSpace = false;


	// false 면 WorldUIPass 에서 배경/채움 sprite draw 를 스킵 
	// (HUD가 자체 UISpriteComponent 로 이미 바를 그리는 경우 중복 방지). 파편/hit effect 만 그림.
	bool mRenderBgFill = true;


	// Hit effect
	// 비어있으면 비활성. 
	std::wstring mHitEffectTextureName;
	int mHitEffectCols = 1;
	int mHitEffectRows = 1;
	int mHitEffectFrameCount = 1;
	// hit 스프라이트 높이 = 바의 보이는 높이(mHeight × fillUvRangeY 폭) × 이 배수 (기본 정사각).
	// 절대 픽셀이 아니라 바에 비례하므로 바 크기가 바뀌어도 자동으로 맞춰진다.
	float mHitEffectHeightScale = 2.0f;
	// hit 스프라이트 가로(U)에 추가되는 폭 = 잃은 피 폭 × 이 배수.
	// 0이면 정사각, 클수록 피해량에 따라 가로로 더 늘어나 "피 깎임"이 잘 보인다.
	float mHitEffectWidthLossScale = 1.0f;

	// 앵커(피격 경계점) 기준 추가 오프셋 — 양수 X = 화면 오른쪽 / 양수 Y = 아래쪽.
	Vec2 mHitEffectOffsetPx = Vec2::Zero;
};
