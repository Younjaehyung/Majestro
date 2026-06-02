#pragma once
#include "System.h"
#include "SocketFollowComponent.h"

class TransformComponent;
class VfxComponent;

class SocketFollowSystem : public System
{
public:
	SocketFollowSystem(World* world);

	void Update(float deltaTime) override;
	std::vector<std::type_index> After() const override;

private:
	bool IsFollowActive(Entity sourceEntity, const SocketFollowComponent& follow) const;
	bool IsAttackStateActive(Entity sourceEntity) const;
	bool IsAnimationWindowActive(Entity sourceEntity, const SocketFollowComponent& follow) const;
	void SetHiddenTransform(TransformComponent& transform, const SocketFollowComponent& follow) const;
	void ApplySocketTransform(TransformComponent& transform, const Matrix& socketMatrix, const SocketFollowComponent& follow) const;
	void UpdateVfxPlayback(VfxComponent* vfx, bool active, SocketFollowComponent& follow) const;
};
