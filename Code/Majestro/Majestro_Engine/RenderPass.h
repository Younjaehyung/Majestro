#pragma once
#include "RenderTarget.h"

class ToneMapPass;
class FinalCompositePass;
class OutlinePass;
struct DrawBatch;
class  CameraComponent;
class World;

// ──────────────────────────────────────────────────────────────
// 컬러 그레이딩 파라미터 (ToneMapPass 에서 사용)
// ──────────────────────────────────────────────────────────────
struct ColorGradingParams
{
	float  Saturation        = 1.15f;  // 채도 (1.0 = 기본, 0.0 = 흑백)
	float  Contrast          = 1.08f;  // 대비 (1.0 = 기본)
	float  Brightness        = 0.0f;  // 밝기 보정 (0.0 = 기본)
	float  Exposure          = 1.0f;  // 노출 보정 (1.0 = 기본, 톤매핑 전 HDR 배율)

	Vec3   ShadowTint        = { 0.0f, 0.01f, 0.02f }; // 어두운 영역 색조 RGB 오프셋
	float  ShadowStrength    = 0.12f;

	Vec3   MidtoneTint       = { 0.01f, 0.005f, 0.0f }; // 중간 영역 색조 RGB 오프셋
	float  MidtoneStrength   = 0.08f;

	Vec3   HighlightTint     = { 0.015f, 0.005f, 0.0f }; // 밝은 영역 색조 RGB 오프셋
	float  HighlightStrength = 0.05f;
};

inline GBUFFER_INDEX ToGBufferIndex(RENDER_TARGET_GROUP_TYPE type, uint32 subRtIndex = 0)
{
	switch (type)
	{
	case RENDER_TARGET_GROUP_TYPE::PRE_DEPTH:  return GBUFFER_INDEX::GBUFFER_PREDEPTH_INDEX;
	case RENDER_TARGET_GROUP_TYPE::G_BUFFER:   return static_cast<GBUFFER_INDEX>(static_cast<uint8>(GBUFFER_INDEX::GBUFFER_POSITION_INDEX) + subRtIndex);
	case RENDER_TARGET_GROUP_TYPE::LIGHTING:   return static_cast<GBUFFER_INDEX>(static_cast<uint8>(GBUFFER_INDEX::GBUFFER_DIFFUSE_INDEX) + subRtIndex);
	case RENDER_TARGET_GROUP_TYPE::HDR:        return GBUFFER_INDEX::GBUFFER_HDR_INDEX;
	case RENDER_TARGET_GROUP_TYPE::POST_HDR_A: return GBUFFER_INDEX::GBUFFER_POSTA_INDEX;
	case RENDER_TARGET_GROUP_TYPE::POST_HDR_B: return GBUFFER_INDEX::GBUFFER_POSTB_INDEX;
	case RENDER_TARGET_GROUP_TYPE::POST_LDR_A: return GBUFFER_INDEX::GBUFFER_POSTC_INDEX;
	case RENDER_TARGET_GROUP_TYPE::POST_LDR_B: return GBUFFER_INDEX::GBUFFER_POSTD_INDEX;
	case RENDER_TARGET_GROUP_TYPE::MOTION_VECTOR: return GBUFFER_INDEX::GBUFFER_MOTIONVEC_INDEX;
	case RENDER_TARGET_GROUP_TYPE::SHADOW:     return GBUFFER_INDEX::GBUFFER_CASCADE_INDEX;
	

	default:                                   return GBUFFER_INDEX::GBUFFER_INDEX_END;
	}
}




struct RenderContext
{
	std::vector<DrawBatch>* deferredBatchs = nullptr;  // 일반 렌더 배치 (GBuffer/Forward)
	std::vector<DrawBatch>* lightBatchs = nullptr;     // 라이트 배치

	// cascade별 그림자 배치 — 각 인덱스에는 해당 cascade 구체와 교차하는 오브젝트만 포함
	std::array<std::vector<DrawBatch>, 4>* cascadeBatchs = nullptr;

	std::array<bool, 4>* cascadeActive = nullptr;  // 활성 캐스케이드 플래그

	CameraComponent* camera = nullptr;
	float                         deltaTime = 0.f;
	uint32                        frameIndex = 0;
};


class IRenderPipeline
{
public:
	virtual ~IRenderPipeline() = default;


	virtual void Initialize(World* world) = 0;


	virtual void OnResize(uint32 width, uint32 height) {}

	// PushPassData()가 GPU 업로드 전에 호출 — 각 Pass의 SetData() 담당
	virtual void SetupPassTable(
		std::array<PassCustomData,
		static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& table) = 0;

	// Compute Shader 사전 연산 | RenderSystem::PreProcess() 역할
	virtual void PreCompute(const RenderContext& ctx) {}
	virtual void Execute(const RenderContext& ctx) = 0;
};


class RenderPass
{
public:
	RenderPass() = default;
	virtual ~RenderPass() = default;
	virtual void Initialize();
	virtual void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable, 
		RENDER_TARGET_GROUP_TYPE before, RENDER_TARGET_GROUP_TYPE after);
	virtual void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

	virtual bool IsEnabled() const { return mEnabled; }
	virtual void SetEnabled(bool enabled) { mEnabled = enabled; }
protected:
	RENDER_TARGET_GROUP_TYPE mBefore;
	RENDER_TARGET_GROUP_TYPE mAfter;


	bool mEnabled = true;
};

class PostProcessPass
{
	// 모든 PostProcessPass는 HDR_POST -> ToneMapPass -> LDR_POST FinalCompositePass 순으로 실행된다고 가정
public:
	PostProcessPass() = default;
	~PostProcessPass() = default;
  
  void Initialize();
  void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>&);
  void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

  void AddHDRPass(shared_ptr<RenderPass> pass) { mHDRPasses.push_back(pass); }
  void AddLDRPass(shared_ptr<RenderPass> pass) { mLDRPasses.push_back(pass); }
  void RemoveHDRPass(shared_ptr<RenderPass> pass) {
      auto it = std::find(mHDRPasses.begin(), mHDRPasses.end(), pass);
      if (it != mHDRPasses.end()) mHDRPasses.erase(it);
  }
  void RemoveLDRPass(shared_ptr<RenderPass> pass) {
      auto it = std::find(mLDRPasses.begin(), mLDRPasses.end(), pass);
      if (it != mLDRPasses.end()) mLDRPasses.erase(it);
  }

  void SetColorGrading(const ColorGradingParams& params);
  const ColorGradingParams& GetColorGrading() const;

private:
	RENDER_TARGET_GROUP_TYPE mLDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mLDRAfterGroupType;

	RENDER_TARGET_GROUP_TYPE mHDRBeforeGroupType; 
	RENDER_TARGET_GROUP_TYPE mHDRAfterGroupType;


	vector<shared_ptr<RenderPass>> mHDRPasses;
	vector<shared_ptr<RenderPass>> mLDRPasses;

	std::shared_ptr<ToneMapPass> mToneMapPass;
	std::shared_ptr<FinalCompositePass> mFinalCompositePass;
};

