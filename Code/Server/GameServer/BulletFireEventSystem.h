#pragma once
#include "System.h"
#include "BulletComponent.h"

class InputComponent;
class TransformComponent;

class BulletFireEventSystem : public System
{
public:
	BulletFireEventSystem(World* world);
	void Update(float dt) override;

private:
	void ActivateBulletAndNotify(Entity playerEntity, SkillType bulletType, bool isCritical);
	std::vector<uint32> CollectPlayerSessions() const;

	Vec3 GetCameraForwardFromInput(const InputComponent& input);
	Vec3 SafeHorizontalForward(Vec3 forward);
	Vec3 SafeRightFromForward(const Vec3& forward);
	Vec3 CalculateServerTpsCameraPosition(const TransformComponent& shooterTransform, const Vec3& cameraForward);
	Vec3 CalculateServerMuzzlePosition(const TransformComponent& shooterTransform, const Vec3& cameraForward);

private:
	static constexpr float kMaxAimDistance = 5000.0f;

	static constexpr float kCameraRightOffset = 80.0f;
	static constexpr float kCameraUpOffset = 160.0f;
	static constexpr float kCameraBackDistance = 250.0f;

	static constexpr float kMuzzleRightOffset = 35.0f;
	static constexpr float kMuzzleUpOffset = 110.0f;
	static constexpr float kMuzzleForwardOffset = 60.0f;

};
