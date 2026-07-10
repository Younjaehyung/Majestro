#pragma once
#include "Component.h"

enum class UIFontType
{
	Arial,
	Esamanru,
	Rivera,
	Count
};

class UITextComponent : public Component<UITextComponent>
{
public:
	UITextComponent() = default;
	~UITextComponent() = default;

	std::function<void()> mOnTextChanged;  // 텍스트 변경 시 호출되는 콜백 함수
public:
	std::shared_ptr<DirectX::SpriteFont> mFont;
	std::wstring mText;
	DirectX::SimpleMath::Vector2 mFontPos;
	UIFontType mFontType{ UIFontType::Arial };
	float mRotation = 0.f; // 라디안 (pivot 기준 회전)
	bool mVisible = true; // false 이면 UIRenderSystem 텍스트 렌더 스킵

	DirectX::XMVECTORF32 mColor{ { { 1.f, 1.f, 1.f, 1.f } } }; // RGBA

	// 외곽선(아웃라인)
	float mOutlineThickness{ 0.f }; // 픽셀 단위 외곽선 두께 (0이면 비활성)
	DirectX::XMVECTORF32 mOutlineColor{ { { 0.f, 0.f, 0.f, 1.f } } }; // 외곽선 RGB (알파는 본체 따라감)
};

