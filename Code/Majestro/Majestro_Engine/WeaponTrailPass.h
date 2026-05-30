#pragma once
#include "RenderPass.h"

class Shader;
class WeaponTrailComponent;

class WeaponTrailPass
{
public:
	void Initialize(World* world);
	void Execute(const RenderContext& ctx);

private:

	void BuildTrailMesh(const WeaponTrailComponent& trail, std::vector<Vertex>& vertices, std::vector<uint32>& indices) const;

private:
	World* mWorld = nullptr;
	shared_ptr<Shader> mShader;
};
