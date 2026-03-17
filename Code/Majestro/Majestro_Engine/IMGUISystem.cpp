#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
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
    ImGui::Begin("Inspector");

	//std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<IMGUIComponent>() };

 //   for (auto& entity : entitys)
 //   {
 //       IMGUIComponent* comp = mWorld->GetComponent<IMGUIComponent>(entity);
 //       if (ImGui::CollapsingHeader(comp->GetName()))
 //       {
 //           std::vector<EditorProperty> props;
	//		
 //           comp->RegisterEditorProperties(props);

 //           for (auto& prop : props)
 //               DrawProperty(prop);
 //       }
 //   }

    // AudioVisualizerIMGUI — Component<AudioVisualizerIMGUI> 직접 상속이므로 별도 순회
#ifdef _IMGUI
    if (mWorld->HasComponentPool<AudioVisualizerIMGUI>())
    {
        for (auto& entity : mWorld->GetEntitiesWithComponent<AudioVisualizerIMGUI>())
        {
            AudioVisualizerIMGUI* comp = mWorld->GetComponent<AudioVisualizerIMGUI>(entity);
            if (!comp) continue;
            comp->Draw();
            if (ImGui::CollapsingHeader("Audio Visualizer"))
            {
                for (auto& prop : comp->mProps)
                    DrawProperty(prop);
            }
        }
    }
#endif

    ImGui::End();

}

void IMGUIRenderSystem::DrawProperty(EditorProperty& prop)
{
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
}

