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
	void Initialize();
	void Update();

private:
    void UploadInstanceBuffer();
	void InstancingRender();

private:
	shared_ptr<Mesh> mQuadMesh;
	std::vector<UIInstanceData> mInstances;
};
