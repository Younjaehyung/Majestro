#include "pch.h"
#include "UISpriteComponent.h"
#include "Texture.h"

UISpriteComponent::UISpriteComponent(shared_ptr<Texture> texture)
{
	
	mTexture = texture;
	if (mTexture)
	{
		mSize = { mTexture->GetWidth(), mTexture->GetHeight() };
		mFrameSize = mSize;
	}
}

UISpriteComponent::UISpriteComponent(shared_ptr<Texture> texture, const Vec2& frameSize, int frameCount, float animationTime)
	: UISpriteComponent(texture)
{
	EnableSpriteSheetAnimation(frameSize, frameCount, animationTime);
}

void UISpriteComponent::EnableSpriteSheetAnimation(const Vec2& frameSize, int frameCount, float animationTime, int startFrame)
{
	mIsAnimated = true;
	mAnimationLoopTime = max(animationTime, 0.001f);
	mFrameSize = frameSize;
	mFrameCount = max(frameCount, 1);
	mAnimationUpdateTime = 0.f;
	SetCurrentFrame(startFrame);

	if (mSize.x <= 0.f || mSize.y <= 0.f)
	{
		mSize = mFrameSize;
	}
}

void UISpriteComponent::SetCurrentFrame(int frameIndex)
{
	mCurrentFrame = std::clamp(frameIndex, 0, max(mFrameCount - 1, 0));
}

RECT UISpriteComponent::GetCurrentFrameRect() const
{
	if (!mTexture)
		return RECT{ 0, 0, 0, 0 };

	const int frameWidth = static_cast<int>(max(mFrameSize.x, 1.f));
	const int frameHeight = static_cast<int>(max(mFrameSize.y, 1.f));
	const int texWidth = static_cast<int>(max(mTexture->GetWidth(), 1.f));
	const int texHeight = static_cast<int>(max(mTexture->GetHeight(), 1.f));
	const int columns = max(texWidth / frameWidth, 1);

	const int frame = std::clamp(mCurrentFrame, 0, max(mFrameCount - 1, 0));
	const int col = frame % columns;
	const int row = frame / columns;

	RECT result{};
	result.left = col * frameWidth;
	result.top = row * frameHeight;
	result.right = min(result.left + frameWidth, texWidth);
	result.bottom = min(result.top + frameHeight, texHeight);

	return result;
}