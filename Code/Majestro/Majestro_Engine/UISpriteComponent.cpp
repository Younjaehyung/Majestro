#include "pch.h"
#include "UISpriteComponent.h"
#include "Texture.h"

UISpriteComponent::UISpriteComponent(shared_ptr<Texture> texture)
{
	
	mTexture = texture;
	if (mTexture)
	{
		mAnimSize = { mTexture->GetWidth(), mTexture->GetHeight() };
		mFrameSize = mAnimSize;
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

	if (mAnimSize.x <= 0.f || mAnimSize.y <= 0.f)
	{
		mAnimSize = mFrameSize;
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

	int frameWidth = static_cast<int>(max(mFrameSize.x, 1.f));
	int frameHeight = static_cast<int>(max(mFrameSize.y, 1.f));
	int texWidth = static_cast<int>(max(mTexture->GetWidth(), 1.f));
	int texHeight = static_cast<int>(max(mTexture->GetHeight(), 1.f));
	const int columns = max(texWidth / frameWidth, 1);

	const int frame = std::clamp(mCurrentFrame, 0, max(mFrameCount - 1, 0));
	const int col = frame % columns;
	const int row = frame / columns;

	RECT result{};
	result.left = col * frameWidth;
	result.top = row * frameHeight;
	result.right = std::min(int(result.left) + frameWidth, texWidth);
	result.bottom = std::min(int(result.top) + frameHeight, texHeight);

	return result;
}


void UISpriteComponent::SetVisibleRangeNormalized(float startX, float endX)
{
	mUseVisibleRange = true;
	mVisibleRangeUsePixels = false;
	mVisibleStartX = std::clamp(startX, 0.f, 1.f);
	mVisibleEndX = std::clamp(endX, 0.f, 1.f);
	if (mVisibleEndX < mVisibleStartX)
		std::swap(mVisibleStartX, mVisibleEndX);
}

void UISpriteComponent::SetVisibleRangePixels(float startPx, float endPx)
{
	mUseVisibleRange = true;
	mVisibleRangeUsePixels = true;
	mVisibleStartX = max(startPx, 0.f);
	mVisibleEndX = max(endPx, 0.f);
	if (mVisibleEndX < mVisibleStartX)
		std::swap(mVisibleStartX, mVisibleEndX);
}

void UISpriteComponent::SetVisibleRangeKeepDestinationSize(bool keepSize)
{
	mVisibleRangeKeepDestinationSize = keepSize;
}

void UISpriteComponent::ClearVisibleRange()
{
	mUseVisibleRange = false;
	mVisibleRangeUsePixels = false;
	mVisibleRangeKeepDestinationSize = false;
	mVisibleStartX = 0.f;
	mVisibleEndX = 1.f;
}