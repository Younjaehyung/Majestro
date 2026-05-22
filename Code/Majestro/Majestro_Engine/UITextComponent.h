#pragma once
#include "Component.h"

enum class UIFontType
{
	Arial,
	Esamanru,
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
	bool mVisible = true; // false 이면 UIRenderSystem 텍스트 렌더 스킵
};

