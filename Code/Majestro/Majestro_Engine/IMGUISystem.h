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
    void DrawLightInspector();         // 방향광(태양) 방향/색상 — UE Pitch/Yaw 로 편집
    void DrawLobbyCharacterTransformInspector();
    void DrawLobbyRoomUIInspector();   // 로비 룸 UI
    
    void DrawWeaponTrailInspector();	//검기/트레일 VFX 파라미터

private:

};
