#pragma once
#include "RenderPass.h"
#include "RenderTarget.h"

class HealthVignettePass : public RenderPass
{
public:
	HealthVignettePass() = default;
	~HealthVignettePass() = default;

	void Initialize() override;
	void SetFeedbackState(float lowHealthStrength, float healStrength, const Vec4& buffIconStrengths);
	void SetNoiseTextureName(const std::wstring& textureName) { mNoiseTextureName = textureName; }
	void SetEffectAtlasTextureName(const std::wstring& textureName) { mEffectAtlasTextureName = textureName; }

	void SetData(
		std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)>& dataTable,
		RENDER_TARGET_GROUP_TYPE before,
		RENDER_TARGET_GROUP_TYPE after) override;

	void Execute(std::vector<DrawBatch>& deferredDrawBatchs) override;

private:
	static constexpr uint32 HEALTH_VIGNETTE_IDX = static_cast<uint32>(PASS_CUSTOM_INDEX::POST_HEALTH_VIGNETTE_PASS);

	float mLowHealthStrength = 0.0f;
	float mHealStrength = 0.0f;
	Vec4 mBuffIconStrengths = Vec4::Zero;

	std::wstring mNoiseTextureName = L"NoiseTex";
	std::wstring mEffectAtlasTextureName = L"UI_Effect_Sheet";
};
