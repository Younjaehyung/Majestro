#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "AudioVisualizerComponent.h"

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

