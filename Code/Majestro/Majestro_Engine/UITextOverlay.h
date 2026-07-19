#pragma once
#include <functional>
#include <memory>
#include "UITextComponent.h"	// UIFontType

namespace DirectX { class SpriteBatch; class SpriteFont; }

struct UITextOverlayContext
{
	DirectX::SpriteBatch* batch = nullptr;	// TextUpdate 가 Begin() 한 배치
	Vec2 screenSize{};
	std::function<std::shared_ptr<DirectX::SpriteFont>(UIFontType)> getFont;
};

using UITextOverlay = std::function<void(const UITextOverlayContext&)>;
