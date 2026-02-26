#pragma once
#include "System.h"
#include "World.h"
#include "ComponentPool.h"
#include "Mesh.h"


struct UIInstanceData
{
    Vec2	Position;  // finalPixelPos
    Vec2	Size;
    Vec2	Pivot;
	uint32  MaterialIndex;
    float   ZOrder;
};


class UIRenderSystem : public System
{
public:
	UIRenderSystem(World* world);
	virtual ~UIRenderSystem();
	void Initialize();
	void InitializeFont();

	void Update();
	void CustomSpriteUpdate();
	void SpriteUpdate();
	void TextUpdate();

private:
    void UploadInstanceBuffer();
	void InstancingRender();

private:

	shared_ptr<Mesh> mQuadMesh;
	std::shared_ptr<DirectX::SpriteBatch> mSpriteBatch;
	std::shared_ptr<DirectX::SpriteFont> mDefaultFont;
	std::vector<UIInstanceData> mInstances;
};
