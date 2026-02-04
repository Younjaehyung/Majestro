#include "pch.h"
#include "UIRenderSystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "Mesh.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UITextComponent.h"
#include "CameraComponent.h"
#include "TagComponent.h"

UIRenderSystem::UIRenderSystem(World* world) : System::System(world)
{
}

void UIRenderSystem::Initialize()
{
	mQuadMesh = RESOURCEMANAGER.Get<Mesh>(L"UIQuad");
}

void UIRenderSystem::InitializeFont()
{
 //   D3D12_CPU_DESCRIPTOR_HANDLE cpuhandle = Graphics_DescHeap->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
 //   uint32 srvSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 //   D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuhandle, (static_cast<uint32>(UI_INDEX_START))*srvSize);
	//D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(Graphics_DescHeap->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart(), (static_cast<uint32>(UI_INDEX_START)) * srvSize);
 //   
 //   DirectX::ResourceUploadBatch resourceUpload(DEVICE.Get());
 //   resourceUpload.Begin();

 //   for (Entity a : mWorld->View<UITextComponent>()) {
 //       auto textComp = mWorld->GetComponent<UITextComponent>(a);

 //           
	//	textComp->m_font = std::make_shared<SpriteFont>(DEVICE.Get(), resourceUpload,L"..Resources\Font\myfile.spritefont", cpuDescriptor, gpuDescriptor);
 //       //textComp->m_font.reset();

 //       auto size = RENDERMANAGER.GetWindow();
 //       textComp->m_fontPos.x = float(size.Width) / 2.f;
 //       textComp->m_fontPos.y = float(size.Height) / 2.f;
 //   }

    //resourceUpload.End(C);
}

void UIRenderSystem::Update()
{
    if (false == mWorld->HasComponentPool<MainCameraComponent>())return;

    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

    if (RENDERMANAGER.IsMsaaEnabled()) {//msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::FINAL)).WaitResourceToTarget();
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::FINAL)).OMSetRenderTargets(1, backIndex);
    }
    else
    {
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
    }

    
    SpriteUpdate();
    //TextUpdate();

    if (RENDERMANAGER.IsMsaaEnabled()) {//msaa
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::FINAL)).WaitTargetToResource();
    }
}

void UIRenderSystem::TextUpdate()
{
    std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<UITextComponent>() };

    for (Entity a : entitys) {
        auto textComp = mWorld->GetComponent<UITextComponent>(a);

        std::wstring output = std::wstring(L"Hello") + std::wstring(L" World");

        textComp->m_spriteBatch->Begin(GRAPHICS_CMD_LIST.Get());

       // const wchar_t* output = L"Hello World";

        Vec2 origin = textComp->m_font->MeasureString(output.c_str()) / 2.f;

        textComp->m_font->DrawString(textComp->m_spriteBatch.get(), output.c_str(),
            textComp->m_fontPos, Colors::White, 0.f, origin);

        textComp->m_spriteBatch->End();
    }
}

void UIRenderSystem::SpriteUpdate()
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
