#pragma once
#include "Component.h"

class HeightField;

struct TerrainParams {
	int TileCountX;
	int TileCountZ;
	Vec2 MinMaxTessDistance;

	float Padding0;
	float MaxTessLevel;
	Vec2 HeightMapResolution;
};
//
//// 월드에서 실제 크기
//float mWorldSizeX = 1.f;
//float mWorldSizeZ = 1.f;
//
//// 월드에서 실제 시작 위치
//float mOriginX = 0.f;
//float mOriginZ = 0.f;
//
//
//// 정규화 높이 범위 (데이터 값을 월드 높이로 바꾸는 스케일)
//float mMinY = 0.f;
//float mMaxY = 100.f;

class TerrainComponent : public Component<TerrainComponent>
{
public:
		TerrainComponent() {}
		TerrainComponent(float width, float depth, std::shared_ptr<HeightField> heightField);

		float GetHeightAtWorldPosition(const Vec3& worldPos) const;

		float GetHeightAtUV(float u, float v) const;

public:
	TerrainParams mTerrainParams = {};
	shared_ptr< HeightField> mHeightField = nullptr;
	Vec3 mTerrainWorldPosition;  // Terrain의 월드 위치
	Vec3 mTerrainWorldScale;     // Terrain의 월드 스케일
};
