#pragma once
#include "Component.h"
#include "Material.h"
class Material;
class Texture;

class UICusSpriteComponent : public Component<UICusSpriteComponent>
{
public:
	UICusSpriteComponent() = default;
	~UICusSpriteComponent() = default;
	UICusSpriteComponent(shared_ptr<Material> material)
	{
		mMaterial = material;
	}

public :


	bool mVisible{ true };
	bool mUIVisibility = true;
	shared_ptr<Material> mMaterial;
};

class UISpriteComponent : public Component<UISpriteComponent>
{
public:
	UISpriteComponent() = default;
	UISpriteComponent(shared_ptr<Texture> texture);
	UISpriteComponent(shared_ptr<Texture> texture, const Vec2& frameSize, int frameCount, float animationTime);
	UISpriteComponent(std::vector<shared_ptr<Texture>>& textures, float animationTime);
	UISpriteComponent(std::vector<shared_ptr<Texture>>& textures);


	~UISpriteComponent() = default;

	void EnableSpriteSheetAnimation(const Vec2& frameSize, int frameCount, float animationTime, int startFrame = 0);
	void SetCurrentFrame(int frameIndex);
	RECT GetCurrentFrameRect() const;

	// 아틀라스 텍스처용
	void SetSourceRect(float x, float y, float width, float height);
	void ClearSourceRect();

	void SetVisibleRangeNormalized(float startX, float endX);
	void SetVisibleRangePixels(float startPx, float endPx);
	void SetVisibleRangeKeepDestinationSize(bool keepSize);
	void ClearVisibleRange();
public :

	bool mIsAnimated{ false };
	float mAnimationUpdateTime = 0.f;
	float mAnimationLoopTime = 0.f;
	float mAnimationSpeed = 1.f;
	
	int mCurrentFrame = 0;
	int mFrameCount = 1;
	Vec2 mFrameSize = { 0.f, 0.f };
	Vec2 mAnimSize{ 0.f, 0.f };


	
	bool mUseSourceRect = false;	// true시 mSourceRect 영역만 그리기
	RECT mSourceRect{ 0, 0, 0, 0 };	// 아틀라스 소스 렉트 크롭 (픽셀 단위)

	// range 크롭(0~1)
	bool mUseVisibleRange = false;
	bool mVisibleRangeUsePixels = false;
	bool mVisibleRangeKeepDestinationSize = false;
	float mVisibleStartX = 0.f;
	float mVisibleEndX = 1.f;
public:
	bool mVisible{ true };
	bool mUIVisibility = true;
	Vec4 mColorTint = Colors::White.v;

	std::vector<shared_ptr<Texture>> mTextures;
	shared_ptr<Texture> mTexture;

	
	
};

