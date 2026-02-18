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

	~UISpriteComponent() = default;


public :


	bool mVisible{ true };
	bool mUIVisibility = true;

	std::shared_ptr<Texture> mTexture;
	Vec2 mPos;
	Vec2 mSize;
	std::shared_ptr<DirectX::SpriteBatch> m_spriteBatch;
};

