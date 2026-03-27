#pragma once
#include "Component.h"
#include "Skeleton.h"
#include "Animator.h"
#include "AnimationGraph.h"

class Material;
class StructuredBuffer;


struct AnimationInstance {
	uint32  EntityID{};
	uint32	SkeletonID{};		// 스켈레톤 핸들(Initialize에서 부여한 값)
	uint32	BoneCount{};		// 본 개수
	uint32	ReulstIndex{};		// CS가 쓰고 VS가 읽는 팔레트 시작 오프셋
	float	UpperLayerWeight{};

	
	// Lower 클립
	uint32	LowerAnimClipID{};		// 현재 재생중인 애니메이션 클립 인덱스
	uint32	LowerCurrentFrame{};		// 현재 프레임
	uint32	LowerNextFrame{};		// 다음 프레임
	float	LowerRatio{};			// 현재 프레임과 다음 프레임 사이의 보간 비율 (0	~1)



	// Lower 블렌드
	uint32	LowerBlendClipIdx{};		// 보간 대상 클립 인덱스
	uint32	LowerBlendCurrentFrame{};
	uint32	LowerBlendNextFrame{};
	float	LowerBlendRatio{};
	float	LowerBlendWeight{};		// 0~1 (0이면 현재 클립만 사용)

	uint32	BlendMaskStart{};	// 블렌드 마스크 시작 본 인덱스
	uint32	BlendMaskEnd{};		// 블렌드 마스크 끝 본 인덱스(포함)
	uint32	BlendMode{};		// 0: Override, 1: Additive

	// Upper 클립
	uint32	UpperAnimClipID{};
	uint32	UpperCurrentFrame{};
	uint32	UpperNextFrame{};
	float	UpperRatio{};

	// Upper 블렌드
	uint32	UpperBlendClipIdx{};
	uint32	UpperBlendCurrentFrame{};
	uint32	UpperBlendNextFrame{};
	float	UpperBlendRatio{};
	float	UpperBlendWeight{};
	
	
	uint32	UpperMaskStart{};
	uint32	UpperMaskEnd{};
	uint32	UpperBlendMode{};
};								
enum class AnimBlendMode : uint32
{
	Override = 0,
	Additive = 1,
};

class AnimationComponent : public Component<AnimationComponent>
{
public:
	AnimationComponent();
	AnimationComponent(vector<shared_ptr<Animator>>& AnimClips);

public:
	uint32							mAnimInstanceID = 0;

	vector<shared_ptr<Animator>>	mAnimClips;
	AnimationInstance				mAnimInstance{};

	// ─── Lower 레이어 (하체 독립 애니메이션) ──────────────────────────────
	uint32							mLowerAnimClipIdx{};		// 현재 재생중인 애니메이션 클립 인덱스
	uint32							mLowerBlendClipIdx{};	// 보간 대상 클립 인덱스

	float							mLowerUpdateTime = 0.f;	// 애니메이션 재생 시간
	float							mLowerBlendUpdateTime = 0.f;
	float							mLowerBlendTimer = 0.f;

	float							mLowerBlendDuration = 0.2f;
	float							mLowerBlendWeight = 0.f;
	AnimBlendCurve					mLowerBlendCurve = AnimBlendCurve::EaseInOut;

	uint32							mLowerBlendMaskStart = 0;	// 상하체 보간용
	uint32							mLowerBlendMaskEnd = 0;
	AnimBlendMode					mLowerBlendMode = AnimBlendMode::Override;

	// ─── Upper 레이어 (상체 독립 애니메이션) ──────────────────────────────

	uint32							mUpperAnimClipIdx{};
	uint32							mUpperBlendClipIdx{};

	float							mUpperUpdateTime = 0.f;
	float							mUpperBlendUpdateTime = 0.f;
	float							mUpperBlendTimer = 0.f;

	float							mUpperBlendDuration = 0.05f;	// 상체 전환 보간 시간
	float							mUpperBlendWeight = 0.f;
	AnimBlendCurve					mUpperBlendCurve = AnimBlendCurve::EaseInOut;


	// Upper 마스크 범위
	uint32							mUpperBlendMaskStart = 0;
	uint32							mUpperBlendMaskEnd = 0;
	AnimBlendMode					mUpperBlendMode = AnimBlendMode::Override;


	bool							mEnableUpperBodyLayer = false;	// 상체 레이어 활성화 여부
	bool							mBoneFinalUpdated = false;

public:


	// ─── AnimationGraph (전환별 duration/curve 관리) ───────────────────────
	// LoadAnimationGraph() 로 JSON 에서 로드한다.
	// nullptr 이면 mBlendDuration / mUpperBlendDuration 고정값을 사용한다.
	shared_ptr<AnimationGraph>		mAnimationGraph;

	// Upper 레이어 가중치 — 0/1 즉각 전환이 아닌 점진적 보간
	float							mUpperLayerWeight = 0.f;		// 현재 가중치 (0~1)
	float							mUpperLayerTargetWeight = 0.f;	// 목표 가중치
	float							mUpperLayerEnterDuration = 0.5f;	// 켜질 때 보간 시간
	float							mUpperLayerExitDuration = 0.3f;	// 꺼질 때 보간 시간
	// AnimationGraph JSON 파일 로드 편의 함수
	void LoadAnimationGraph(const wstring& jsonPath)
	{
		mAnimationGraph = make_shared<AnimationGraph>();
		if (!mAnimationGraph->LoadFromJson(jsonPath))
			mAnimationGraph = nullptr;	// 로드 실패 시 기본값 사용
		else
		{
			// 그래프에서 Enter/Exit 시간도 동기화
			mUpperLayerEnterDuration = mAnimationGraph->GetUpperEnterDuration();
			mUpperLayerExitDuration  = mAnimationGraph->GetUpperExitDuration();
		}
	}
};

