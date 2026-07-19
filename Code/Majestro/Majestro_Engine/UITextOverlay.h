#pragma once
#include <functional>
#include <memory>
#include <DirectXTK12/SpriteBatch.h>
#include <DirectXTK12/SpriteFont.h>	
#include "UITextComponent.h"		

struct UITextOverlayContext
{
	DirectX::SpriteBatch* batch = nullptr;	// TextUpdate 가 Begin() 한 배치
	Vec2 screenSize{};
	std::function<std::shared_ptr<DirectX::SpriteFont>(UIFontType)> getFont;
};

using UITextOverlay = std::function<void(const UITextOverlayContext&)>;
