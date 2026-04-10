#pragma once
#include "System.h"
#include <string>
#include "World.h"
#include "Imgui.h"
#include "IMGUIComponent.h"


class IMGUIRenderSystem : public System
{
public:
	IMGUIRenderSystem(World* world);
	void Initialize();
	void Update();

private:
    void UploadInstanceBuffer();
	void DrawProperty(EditorProperty& prop);
    void DrawCameraInspector();

private:

};
