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
	std::shared_ptr<DirectX::SpriteFont> m_font;

	DirectX::SimpleMath::Vector2 m_fontPos;
	std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch;
};

