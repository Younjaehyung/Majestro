#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"

IMGUIRenderSystem::IMGUIRenderSystem(World* world) : System::System(world)
{
}

void IMGUIRenderSystem::Initialize()
{
}

void IMGUIRenderSystem::Update()
{
    ImGui::Begin("Inspector");

	std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<IMGUIComponent>() };

    for (auto& entity : entitys)
    {
        IMGUIComponent* comp = mWorld->GetComponent<IMGUIComponent>(entity);
        if (ImGui::CollapsingHeader(comp->GetName()))
        {
            std::vector<EditorProperty> props;
			
            comp->RegisterEditorProperties(props);

            for (auto& prop : props)
                DrawProperty(prop);
        }
    }

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

        case PropertyType::Vec3:
            ImGui::DragFloat3(
                prop.name.c_str(),
                (float*)prop.data,
                0.1f
            );
            break;
        }
}

