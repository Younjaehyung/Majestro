#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "AudioVisualizerComponent.h"
#include "CameraComponent.h"
#include "SystemManager.h"

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
    }

    // ── Camera Inspector ──────────────────────────────────────────────────────
    DrawCameraInspector();
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

