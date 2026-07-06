#pragma once
#include "PacketHelper.h"

class SceneManager;

// C2S_SCENE_CHANGE 파싱 → SceneManager::TryChangeScene → 응답 송신
class ScenePacketHandler
{
public:
    explicit ScenePacketHandler(SceneManager* sceneManager) : mSceneManager(sceneManager) {}

    bool Handle(const InputCommand& command);

private:
    SceneManager* mSceneManager = nullptr;
};
