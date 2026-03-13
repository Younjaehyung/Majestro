#pragma once
#include "RenderTarget.h"
#include "RenderSystem.h"
class ToneMapPass;
class FinalCompositePass;
class OutlinePass;

// RENDER_TARGET_GROUP_TYPE → GBUFFER_INDEX 변환
// subRtIndex: G_BUFFER(0=Position,1=Normal,2=Color), LIGHTING(0=Diffuse,1=Specular) 등 멀티 RT 그룹에서의 내부 인덱스
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
	case RENDER_TARGET_GROUP_TYPE::SHADOW:     return GBUFFER_INDEX::GBUFFER_CASCADE_INDEX;
	default:                                   return GBUFFER_INDEX::GBUFFER_INDEX_END;
	}
}


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
public:
	PostProcessPass() = default;
	~PostProcessPass() = default;
  
  void Initialize();
  void SetData(std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>&);
  void Execute(std::vector<DrawBatch>& deferredDrawBatchs);

  void AddHDRPass(shared_ptr<RenderPass> pass) { mHDRPasses.push_back(pass); }
  void AddLDRPass(shared_ptr<RenderPass> pass) { mLDRPasses.push_back(pass); }

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

