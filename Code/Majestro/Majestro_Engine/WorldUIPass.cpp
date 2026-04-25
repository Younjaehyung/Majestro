#include "pch.h"
#include "WorldUIPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "Mesh.h"
#include "Shader.h"
#include "Texture.h"
#include "World.h"
#include "CameraComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "HealthComponent.h"
#include "UIComponent.h"
#include "UIRenderSystem.h" // UIInstanceData
#include "UITransformComponent.h"

void WorldUIPass::Initialize(World* world)
{
    mWorld = world;
}

void WorldUIPass::Execute(CameraComponent* camera)
{
    if (mFeatures == nullptr)
        return;

    for (const auto& feature : *mFeatures)
    {
        feature->WorldRender(camera);
    }
}