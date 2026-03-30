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
	UISpriteComponent(std::vector<shared_ptr<Texture>>& textures, float animationTime)
	{
		mIsAnimated = true;
		mAnimationLoopTime = animationTime;
		mTexture = textures[0];
		mTextures = textures;
	}
	UISpriteComponent(std::vector<shared_ptr<Texture>>& textures)
	{
		mTexture = textures[0];
		mTextures = textures;
	}


	~UISpriteComponent() = default;

	void EnableSpriteSheetAnimation(const Vec2& frameSize, int frameCount, float animationTime, int startFrame = 0);
	void SetCurrentFrame(int frameIndex);
	RECT GetCurrentFrameRect() const;

public :

	bool mIsAnimated{ false };
	float mAnimationUpdateTime = 0.f;
	float mAnimationLoopTime = 0.f;
	float mAnimationSpeed = 1.f;
	
	int mCurrentFrame = 0;
	int mFrameCount = 1;
	Vec2 mFrameSize = { 0.f, 0.f };
	Vec2 mAnimSize{ 0.f, 0.f };

public:
	bool mVisible{ true };
	bool mUIVisibility = true;

	std::vector<shared_ptr<Texture>> mTextures;
	shared_ptr<Texture> mTexture;

	
	
};

