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
#include "GameRuleComponent.h"
#include "GameMode.h"
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

    const GameRuleComponent* gameRule =
        mWorld->GetSingleton<GameRuleComponent>();
    if (gameRule != nullptr &&
        gameRule->mGamePhase == static_cast<uint8>(WavePhaseType::Fail))
    {
        // 게임 오버 동안 적 체력바 등 GameWorld UI 직접 렌더 경로를 차단한다.
        return;
    }

    for (const auto& feature : *mFeatures)
    {
        feature->WorldRender(camera);
    }
}
