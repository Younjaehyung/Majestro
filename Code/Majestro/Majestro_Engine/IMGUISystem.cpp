#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "AudioVisualizerComponent.h"
#include "CameraComponent.h"
#include "SystemManager.h"
#include "WeaponTrailComponent.h"
#include "SocketComponent.h"

IMGUIRenderSystem::IMGUIRenderSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Render;
	mOrder = 100;  // 다른 Sim 시스템 이후에 실행
}

void IMGUIRenderSystem::Initialize()
{
}

void IMGUIRenderSystem::Update()
{
#ifdef _IMGUI
    ImGui::Begin("Inspector");

    if (mWorld->HasComponentPool<IMGUIComponent>())
    {
        std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<IMGUIComponent>() };

        for (auto& entity : entitys)
        {
            IMGUIComponent* comp = mWorld->GetComponent<IMGUIComponent>(entity);
            if (!comp) continue;

            if (ImGui::CollapsingHeader(comp->GetName()))
            {
               

                for (auto& prop : comp->mProps)
                    DrawProperty(prop);
            }
        }
    }


    ImGui::End();

    // 현재 활성 파이프라인의 Pass on/off 및 파라미터 조절 창
    if (auto* renderSys = mWorld->GetSystemManager()->GetSystem<RenderSystem>())
    {
        if (auto pipeline = renderSys->GetPipeline())
            pipeline->DrawImGui();

        // CSM 스플릿 분포 lambda
        ImGui::SliderFloat("CSM Split Lambda", renderSys->GetCascadeSplitLambdaPtr(), 0.0f, 1.0f);
    }

    // ── Camera Inspector ──────────────────────────────────────────────────────
    DrawCameraInspector();

    // ── Weapon Trail Inspector ─────────────────────────────────────────────────
    DrawWeaponTrailInspector();
#endif
}

void IMGUIRenderSystem::DrawCameraInspector()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Camera Inspector"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<CameraComponent>())
    {
        ImGui::Text("CameraComponent 없음");
        ImGui::End();
        return;
    }

    auto cameras = mWorld->GetEntitiesWithComponent<CameraComponent>();
    int camIdx = 0;

    for (auto e : cameras)
    {
        CameraComponent* cam = mWorld->GetComponent<CameraComponent>(e);
        if (!cam) continue;

        char header[64];
        snprintf(header, sizeof(header), "Camera [%d] (Entity %u)", camIdx++, e);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 투영 타입
            int projType = static_cast<int>(cam->mCameraType);
            const char* projNames[] = { "Perspective", "Orthographic" };
            if (ImGui::Combo("Projection##Cam", &projType, projNames, 2))
                cam->mCameraType = static_cast<PROJECTION_TYPE>(projType);

            ImGui::Separator();
            ImGui::Text("Clip Plane");
            ImGui::DragFloat("Near##Cam",  &cam->mNear, 10.0f, 1.0f, 10000.0f);
            ImGui::DragFloat("Far##Cam",   &cam->mFar,  1000.0f, 1000.0f, 2000000.0f);

            ImGui::Separator();
            ImGui::Text("Shadow Clip");
            ImGui::DragFloat("ShadowNear##Cam", &cam->mShadowNear, 0.1f, 0.1f,  1000.0f);
            ImGui::DragFloat("ShadowFar##Cam",  &cam->mShadowFar,  100.0f, 100.0f, 100000.0f);

            ImGui::Separator();
            if (cam->mCameraType == PROJECTION_TYPE::PERSPECTIVE)
            {
                ImGui::SliderFloat("FOV (deg)##Cam", &cam->mFov, 10.0f, 90.0f);
            }
            else
            {
                ImGui::DragFloat("Scale##Cam", &cam->mScale, 0.01f, 0.01f, 100.0f);
            }

            // CameraTypeComponent (같은 엔티티에 붙어 있을 경우)
            CameraTypeComponent* camType = mWorld->GetComponent<CameraTypeComponent>(e);
            if (camType)
            {
                ImGui::Separator();
                ImGui::Text("CameraTypeComponent");

                const char* modeNames[] = { "MAIN_CAMERA", "1FPS", "3FPS", "3RPG" };
                ImGui::Text("PlayMode: %s", modeNames[static_cast<int>(camType->mPlayMode)]);

                ImGui::DragFloat("MoveSpeed##CamType",    &camType->mCameraMoveSpeed,   1.0f, 0.0f, 2000.0f);
                ImGui::DragFloat("MaxLength##CamType",    &camType->mCameraMaxLenth,    1.0f, 0.0f, 2000.0f);
                ImGui::DragFloat("MinLength##CamType",    &camType->mCameraMinLenth,    0.1f, 0.0f, 500.0f);
                ImGui::DragFloat("SphereRadius##CamType", &camType->mCameraSphereRadius, 0.5f, 0.0f, 200.0f);
                ImGui::DragFloat("Margin##CamType",       &camType->mCameraMargin,       0.1f, 0.0f, 100.0f);
                ImGui::DragFloat3("Offset##CamType",      &camType->mOffset.x,           1.0f);
                ImGui::DragFloat3("LookAtOffset##CamType",&camType->mLookAtOffset.x,     0.1f);
            }
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawWeaponTrailInspector()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Weapon Trail Inspector"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<WeaponTrailComponent>())
    {
        ImGui::Text("WeaponTrailComponent 없음");
        ImGui::End();
        return;
    }

    
    static const char* styleNames[] = { "FlameRibbon", "SwordSlash", "HammerFlame" };
    static const WeaponTrailVisualStyle styleValues[] = {
        WeaponTrailVisualStyle::FlameRibbon,
        WeaponTrailVisualStyle::SwordSlash,
        WeaponTrailVisualStyle::HammerFlame,
    };

    auto entities = mWorld->GetEntitiesWithComponent<WeaponTrailComponent>();
    int trailIdx = 0;

    for (auto e : entities)
    {
        WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(e);
        if (!trail) continue;

        char header[64];
        snprintf(header, sizeof(header), "Trail [%d] (Entity %u)", trailIdx++, e);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(static_cast<int>(e.GetID()));

            ImGui::Checkbox("Active", &trail->mIsActive);

            // 스타일 선택
            int styleSel = 0;
            for (int i = 0; i < IM_ARRAYSIZE(styleValues); ++i)
                if (styleValues[i] == trail->mVisualStyle) styleSel = i;
            if (ImGui::Combo("Style", &styleSel, styleNames, IM_ARRAYSIZE(styleNames)))
                trail->mVisualStyle = styleValues[styleSel];

            // Tip / Base 오프셋
            // tip/base = 트레일 리본의 폭 방향 양 끝. 칼날을 따라 눕도록.
            ImGui::Separator();
            ImGui::Text("Tip / Base Offset");
            if (trail->mSourceType == WeaponTrailSource::Socket)
            {
                // Socket 모드
                const Entity src = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : e;
                SocketComponent* socketCom = mWorld->GetComponent<SocketComponent>(src);
                if (socketCom != nullptr)
                {
                    if (SocketDef* tipSocket = socketCom->FindSocket(trail->mTipSocketName))
                    {
                        Vec3 tipPos = tipSocket->mLocalOffset.Translation();
                        if (ImGui::DragFloat3("TipOffset", &tipPos.x, 0.5f))
                            tipSocket->mLocalOffset.Translation(tipPos);
                    }
                    else
                        ImGui::TextDisabled("Tip 소켓 '%s' 없음", trail->mTipSocketName.c_str());

                    if (SocketDef* baseSocket = socketCom->FindSocket(trail->mBaseSocketName))
                    {
                        Vec3 basePos = baseSocket->mLocalOffset.Translation();
                        if (ImGui::DragFloat3("BaseOffset", &basePos.x, 0.5f))
                            baseSocket->mLocalOffset.Translation(basePos);
                    }
                    else
                        ImGui::TextDisabled("Base 소켓 '%s' 없음", trail->mBaseSocketName.c_str());
                }
                else
                    ImGui::TextDisabled("SourceEntity에 SocketComponent 없음");
            }
            else
            {
                // Transform 모드
                ImGui::DragFloat3("TipOffset", &trail->mTipLocalOffset.x, 0.5f);
                ImGui::DragFloat3("BaseOffset", &trail->mBaseLocalOffset.x, 0.5f);
            }

            ImGui::Separator();
            ImGui::Text("Shape");
            int layerCount = static_cast<int>(trail->mLayerCount);
            if (ImGui::SliderInt("LayerCount", &layerCount, 1, 6))
                trail->mLayerCount = static_cast<uint32>(layerCount);
            int subdiv = static_cast<int>(trail->mSmoothingSubdivisions);
            if (ImGui::SliderInt("Smoothing", &subdiv, 0, 4))
                trail->mSmoothingSubdivisions = static_cast<uint32>(subdiv);
            ImGui::SliderFloat("WidthMultiplier", &trail->mWidthMultiplier, 0.0f, 4.0f);
            ImGui::SliderFloat("TailWidth", &trail->mTailWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("MidWidth", &trail->mMidWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("HeadWidth", &trail->mHeadWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("LayerSpread", &trail->mLayerSpread, 0.0f, 1.0f);
            ImGui::SliderFloat("Lifetime", &trail->mLifetime, 0.02f, 0.6f);
            ImGui::SliderFloat("UvTiling", &trail->mUvTiling, 0.1f, 8.0f);

            ImGui::Separator();
            ImGui::Text("Slash Shading");
            ImGui::SliderFloat("BaseAlpha", &trail->mBaseAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat("Intensity", &trail->mIntensity, 0.0f, 12.0f);
            ImGui::SliderFloat("CutStrength", &trail->mSlashCutStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("LineStrength", &trail->mSlashLineStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("EdgeBoost", &trail->mSlashEdgeBoost, 0.0f, 4.0f);
            ImGui::SliderFloat("CoreBoost", &trail->mSlashCoreBoost, 0.0f, 4.0f);
            ImGui::SliderFloat("TexScrollSpeed", &trail->mSlashTexScrollSpeed, 0.0f, 4.0f);

            ImGui::Separator();
            ImGui::Text("Colors");
            ImGui::ColorEdit3("CoreColor", &trail->mCoreColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::ColorEdit3("EdgeColor", &trail->mEdgeColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::ColorEdit3("SubColor",  &trail->mSubColor.x,  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

            ImGui::PopID();
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawProperty(EditorProperty& prop)
{
#ifdef _IMGUI
        switch (prop.type)
        {
        case PropertyType::Bool:
            ImGui::Checkbox(prop.name.c_str(), (bool*)prop.data);
            break;

        case PropertyType::Float:
            ImGui::SliderFloat(
                prop.name.c_str(),
                (float*)prop.data,
                prop.min,
                prop.max
            );
            break;

        case PropertyType::Vec2:
            ImGui::DragFloat2(
                prop.name.c_str(),
                (float*)prop.data,
                1.0f
            );
            break;

        case PropertyType::Vec3:
            ImGui::DragFloat3(
                prop.name.c_str(),
                (float*)prop.data,
                0.1f
            );
            break;
        }
#endif
}

