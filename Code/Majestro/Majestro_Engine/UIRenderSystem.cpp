#include "pch.h"
#include "UIRenderSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Mesh.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"

UIRenderSystem::UIRenderSystem(World* world) : System::System(world)
{
}

void UIRenderSystem::Initialize()
{
	mQuadMesh = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
}

void UIRenderSystem::Update()
{
    mInstances.clear();


    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponents<UITransformComponent, UISpriteComponent>() };

    for (auto& e : entitys)
    {
        auto tr = mWorld->GetComponent<UITransformComponent>(e);
        auto sp = mWorld->GetComponent<UISpriteComponent>(e);

        if (!sp->mVisible)
            continue;

        UIInstanceData data;
        data.Position = tr->mFinalPixelPos;
        data.Size = tr->mSize;
        data.Pivot = tr->mPivot;
        data.MaterialIndex = sp->mMaterial->GetIndex();
        data.ZOrder = static_cast<float>(tr->mUILayerIndex);

        mInstances.push_back(data);
    }

    UploadInstanceBuffer();
    InstancingRender();

}

void UIRenderSystem::UploadInstanceBuffer()
{
    uint32 mFrameCount = RENDERMANAGER.GetFrameResourceIndex();
    RENDERMANAGER.GetGroupBuffer(mFrameCount)->UIInfo->PushGraphicsData(mInstances.data(), static_cast<uint32>(sizeof(UIInstanceData) * mInstances.size()));

}

void UIRenderSystem::InstancingRender()
{
    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();


    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);


    RESOURCEMANAGER.Get<Shader>(L"UI")->Update();

    mQuadMesh->Render(mInstances.size(), 0, 0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);

}
