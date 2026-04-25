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

class UIFeature;
class UIEffectPass;

class UIRenderSystem : public System
{
public:
	UIRenderSystem(World* world);
	virtual ~UIRenderSystem();
	void SetFeatures(std::vector<shared_ptr<UIFeature>>* features) { mFeatures = features; }
	
	
	void Initialize();
	void InitializeFont();

	void Update();
	void CustomSpriteRender();
	void SpriteUpdate();
	void PostSpriteRender();
	void TextUpdate();

private:
    
	void RenderCustomSprite();
	void RenderSpirte();
	void RenderText();
	
	
	
	
	void UploadInstanceBuffer();
	void InstancingRender(uint32 count, uint32 startInstance = 0);

private:

	shared_ptr<Mesh> mQuadMesh;
	std::shared_ptr<DirectX::SpriteBatch> mSpriteBatch;
	std::shared_ptr<DirectX::SpriteFont> mDefaultFont;
	std::vector<UIInstanceData> mInstances;
	std::vector<std::shared_ptr<UIFeature>>* mFeatures;
	std::shared_ptr<UIEffectPass> mUIEffectPass;
};
