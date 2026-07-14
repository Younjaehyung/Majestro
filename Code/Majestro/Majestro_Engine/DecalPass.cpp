#include "pch.h"
#include "DecalPass.h"

#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "RenderTarget.h"
#include "Shader.h"
#include "Mesh.h"
#include "World.h"
#include "DecalComponent.h"
#include "CameraComponent.h"

namespace
{
    float ComputeFadeAlpha(const DecalComponent& d)
    {
        float alpha = 1.0f;
        if (d.FadeIn > 0.0f)
            alpha *= std::clamp(d.Elapsed / d.FadeIn, 0.0f, 1.0f);
        if (d.Lifetime >= 0.0f && d.FadeOut > 0.0f)
            alpha *= std::clamp((d.Lifetime - d.Elapsed) / d.FadeOut, 0.0f, 1.0f);
        return alpha;
    }

    // 경계구를 감싸는 AABB로 프러스텀 컬링
    bool IsVisible(CameraComponent* camera, const DecalComponent& d)
    {
        if (camera == nullptr)
            return true;
        const float boundR = std::sqrt(2.0f * d.Radius * d.Radius + d.Height * d.Height);
        BoundingOrientedBox obb;
        obb.Center      = XMFLOAT3(d.Center.x, d.Center.y, d.Center.z);
        obb.Extents     = XMFLOAT3(boundR, boundR, boundR);
        obb.Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
        return camera->IntersectsOBB(obb);
    }
}

void DecalPass::Initialize(World* world)
{
    mWorld = world;
}

void DecalPass::Execute(CameraComponent* camera)
{
    if (!mEnabled || mWorld == nullptr)
        return;
    if (mWorld->HasComponentPool<DecalComponent>() == false)
        return;

    auto entities = mWorld->GetEntitiesWithComponent<DecalComponent>();
    if (entities.empty())
        return;

    // 컬링
    mDraws.clear();
    mDraws.reserve(entities.size());
    for (Entity e : entities)
    {
        DecalComponent* d = mWorld->GetComponent<DecalComponent>(e);
        if (d == nullptr || d->Visible == false)
            continue;
        const float fadeAlpha = ComputeFadeAlpha(*d);
        if (fadeAlpha <= 0.0f)
            continue;
        if (IsVisible(camera, *d) == false)   // 프러스텀 컬링
            continue;
        mDraws.push_back({ d, fadeAlpha });
    }

    if (mDraws.empty())
        return;

    auto shader = RESOURCEMANAGER.Get<Shader>(L"Decal");
    auto cube   = RESOURCEMANAGER.Get<Mesh>(L"Cube");
    if (!shader || !cube)
        return;

    auto& hdrGroup      = RENDERMANAGER.GetRenderTargetGroup(
        static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
    auto* depthResource = hdrGroup.GetDSTexture()->GetTex2D().Get();

    auto toDepthRead = CD3DX12_RESOURCE_BARRIER::Transition(
        depthResource,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthRead);

    hdrGroup.WaitResourceToTarget();
    hdrGroup.OMSetRenderTargetsReadOnlyDepth();

    shader->Update();

    for (const Draw& draw : mDraws)
    {
        const DecalComponent* d = draw.d;

        // DECAL_PARAMS
        float cb[16];
        cb[0]  = d->Center.x; cb[1]  = d->Center.y; cb[2]  = d->Center.z; cb[3]  = d->Radius;
        cb[4]  = d->Normal.x; cb[5]  = d->Normal.y; cb[6]  = d->Normal.z; cb[7]  = d->Height;
        cb[8]  = d->Color.x;  cb[9]  = d->Color.y;  cb[10] = d->Color.z;  cb[11] = d->Color.w;
        cb[12] = draw.fadeAlpha; cb[13] = d->NormalThreshold; cb[14] = d->Thickness;
        cb[15] = static_cast<float>(d->TexIndex);

        GRAPHICS_CMD_LIST->SetGraphicsRoot32BitConstants(0, 16, cb, 0);
        cube->Render();
    }

    hdrGroup.WaitTargetToResource();

    auto toDepthWrite = CD3DX12_RESOURCE_BARRIER::Transition(
        depthResource,
        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    GRAPHICS_CMD_LIST->ResourceBarrier(1, &toDepthWrite);
}
