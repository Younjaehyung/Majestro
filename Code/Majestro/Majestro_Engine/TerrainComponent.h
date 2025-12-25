#pragma once
#include "Component.h"
#include "Material.h"

struct TerrainParams {
	int TileCountX;
	int TileCountZ;
	Vec2 MinMaxTessDistance;

	float Padding0;
	float MaxTessLevel;
	Vec2 HeightMapResolution;
};

class TerrainComponent : public Component<TerrainComponent>
{
public:
		TerrainComponent() {}
		TerrainComponent(float width, float depth, shared_ptr< Material> material);

		float GetHeightAtWorldPosition(const Vec3& worldPos) const;

		float GetHeightAtUV(float u, float v) const;

		// 월드 좌표에서 높이 값 가져오기
		//float GetHeightAtWorldPosition(const Vec3& worldPos) const;

		//float SampleHeightBilinear(float u, float v) const;

		// UV 좌표에서 높이 값 가져오기
		//float GetHeightAtUV(float u, float v) const;
public:
	TerrainParams mTerrainParams = {};
	shared_ptr< Material> mMaterial = nullptr;
	Vec3 mTerrainWorldPosition;  // Terrain의 월드 위치
	Vec3 mTerrainWorldScale;     // Terrain의 월드 스케일
};
