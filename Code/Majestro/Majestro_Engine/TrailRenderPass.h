#pragma once
#include "RenderPass.h"
#include "TrailRenderTypes.h"

class DashSpeedLineComponent;
class Shader;
class WeaponTrailComponent;

class TrailRenderPass
{
public:
	void Initialize(World* world);
	void Execute(const RenderContext& ctx);

private:
	bool BuildDescFromWeaponTrail(const WeaponTrailComponent& trail, TrailRenderDesc& out) const;
	void AppendDescsFromDashSpeedLine(const DashSpeedLineComponent& trail, std::vector<TrailRenderDesc>& out) const;
	void BuildSmoothedSamples(const std::vector<TrailRenderSample>& source, uint32 subdivisions, std::vector<TrailRenderSample>& out) const;
	Vec3 CatmullRom(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) const;
	void BuildTrailMesh(const TrailRenderDesc& desc, std::vector<Vertex>& vertices, std::vector<uint32>& indices) const;

private:
	World* mWorld = nullptr;
	shared_ptr<Shader> mShader;
};
