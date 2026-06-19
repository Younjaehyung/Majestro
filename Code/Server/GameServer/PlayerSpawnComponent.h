#pragma once
#include "Component.h"
#include <array>

// 플레이어 스폰 지점 마커.
class PlayerSpawnComponent : public Component<PlayerSpawnComponent>
{
public:
	Vec3 mPosition{};
	std::array<Vec3, 3> mCharacterPositions{};
};
