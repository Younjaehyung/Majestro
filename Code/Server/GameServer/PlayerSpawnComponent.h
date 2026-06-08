#pragma once
#include "Component.h"

// 플레이어 스폰 지점 마커.
class PlayerSpawnComponent : public Component<PlayerSpawnComponent>
{
public:
	Vec3 mPosition{};
};
