#pragma once
#include "System.h"

class EventManager;

// 플레이어 무적 (서버 전체). -qagod 로 켜서 시작하고, 실행 중에는 MCP server_god 로 바꾼다. 로직 스레드에서만 쓴다.
bool& PlayerGodMode();

class DamageSystem : public System
{
public:
    explicit DamageSystem(World* world);

    void Update(float deltaTime) override;

private:
    void ApplyRudwigCriticalRhythmEffect(Entity instigator, EventManager& eventManager);
	void AddRhythmPoints(Entity player, int32 amount);
};
