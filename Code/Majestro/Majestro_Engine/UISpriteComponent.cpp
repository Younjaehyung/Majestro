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

UISpriteComponent::UISpriteComponent(shared_ptr<Texture> texture, float x, float y, float width, float height)
{
	mTexture = texture;
	SetSourceRect(x, y, width, height);
}

UISpriteComponent::UISpriteComponent(shared_ptr<Texture> texture, const Vec2& frameSize, int frameCount, float animationTime)
	: UISpriteComponent(texture)
{
	EnableSpriteSheetAnimation(frameSize, frameCount, animationTime);
}

UISpriteComponent::UISpriteComponent(std::vector<shared_ptr<Texture>>& textures, float animationTime)
{
	mIsAnimated = true;
	mAnimationLoopTime = animationTime;
	mTexture = textures[0];
	mTextures = textures;
}

UISpriteComponent::UISpriteComponent(std::vector<shared_ptr<Texture>>& textures)
{
	mTexture = textures[0];
	mTextures = textures;
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


void UISpriteComponent::SetSourceRect(float x, float y, float width, float height)
{
	mUseSourceRect = true;
	mSourceRect = RECT{
		static_cast<LONG>(x),
		static_cast<LONG>(y),
		static_cast<LONG>(x + width),
		static_cast<LONG>(y + height)
	};
}

void UISpriteComponent::ClearSourceRect()
{
	mUseSourceRect = false;
	mSourceRect = RECT{ 0, 0, 0, 0 };
}

void UISpriteComponent::SetVisibleRangeNormalizedX(float startX, float endX)
{
	mUseVisibleRange = true;
	mVisibleRangeUsePixels = false;
	mVisibleRangeVertical = false;
	mVisibleStartX = std::clamp(startX, 0.f, 1.f);
	mVisibleEndX = std::clamp(endX, 0.f, 1.f);
	if (mVisibleEndX < mVisibleStartX)
		std::swap(mVisibleStartX, mVisibleEndX);
}

void UISpriteComponent::SetVisibleRangeNormalizedY(float startY, float endY)
{
	mUseVisibleRange = true;
	mVisibleRangeUsePixels = false;
	mVisibleRangeVertical = true;
	mVisibleStartY = std::clamp(startY, 0.f, 1.f);
	mVisibleEndY = std::clamp(endY, 0.f, 1.f);
	if (mVisibleEndY < mVisibleStartY)
		std::swap(mVisibleStartY, mVisibleEndY);
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
	mVisibleRangeVertical = false;
	mVisibleStartX = 0.f;
	mVisibleEndX = 1.f;
	mVisibleStartY = 0.f;
	mVisibleEndY = 1.f;
}