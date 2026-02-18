#pragma once
#include "Component.h"

enum class UIFontType
{
	Arial,
	Count
};

class UITextComponent : public Component<UITextComponent>
{
public:
	UITextComponent() = default;
	~UITextComponent() = default;

	void Update();
public:
	std::shared_ptr<DirectX::SpriteFont> mFont;
	std::wstring mText;
	DirectX::SimpleMath::Vector2 mFontPos;
	UIFontType mFontType{ UIFontType::Arial };
};

