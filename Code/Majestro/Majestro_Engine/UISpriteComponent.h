#pragma once
#include "Component.h"
#include "Material.h"
class Material;

class UISpriteComponent : public Component<UISpriteComponent>
{
public:
	UISpriteComponent() = default;
	~UISpriteComponent() = default;
	UISpriteComponent(shared_ptr<Material> material)
	{
		mMaterial = material;
	}

public :


	bool mVisible{ true };
	bool mUIVisibility = true;
	shared_ptr<Material> mMaterial;
};

