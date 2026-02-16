#include "pch.h"
#include "TerrainComponent.h"
#include "Texture.h"


TerrainComponent::TerrainComponent(int width, int depth, shared_ptr<Material>& material)
{

	mHeightmap = material;
	shared_ptr<Texture> heightMap = material->GetTexture(2);
    mTerrainParams = {

        .TileCountX = width,
	    .TileCountZ = depth,
		.MinMaxTessDistance = Vec2(100.f , 5000.f),


		.MaxTessLevel = 3.0f,
		.HeightMapResolution = Vec2(heightMap->GetWidth() , heightMap->GetHeight())
	};

}

float TerrainComponent::GetHeightAtWorldPosition(const Vec3& worldPos) const
{
    if (!mHeightmap)
        return 0.0f;

    std::shared_ptr<Texture> heightMap = mHeightmap->GetTexture(2);
    if (!heightMap)
        return 0.0f;

    const float tileCountX = static_cast<float>(mTerrainParams.TileCountX); // 504
    const float tileCountZ = static_cast<float>(mTerrainParams.TileCountZ); // 504

    // 수정: 월드 좌표 → 로컬 좌표 변환
    Vec3 localPos = worldPos - mTerrainWorldPosition;

    // 수정: 스케일 적용하여 정규화된 로컬 좌표 계산
    // 메쉬 로컬 공간에서 0~504 범위를 0~1로 변환
    if (mTerrainWorldScale.x == 0.0f || mTerrainWorldScale.z == 0.0f)
        return mTerrainWorldPosition.y;

    // 로컬 좌표를 스케일로 나눈 후, 타일 개수로 정규화
    float normalizedX = (localPos.x / mTerrainWorldScale.x) * tileCountX;
    float normalizedZ = (localPos.z / mTerrainWorldScale.z) * tileCountZ;

    // UV 계산 (0~1 범위)
    float fullU = normalizedX / tileCountX;
    float fullV = 1.0f - (normalizedZ / tileCountZ);

    // 범위 체크
    if (fullU < 0.0f || fullU > 1.0f ||
        fullV < 0.0f || fullV > 1.0f)
    {
        return 0.0f;
    }

    // HeightMap 샘플링
    const float normalizedHeight = heightMap->GetHeightValue(fullU, fullV); // 0~1

    // 최종 월드 높이
    const float worldY = mTerrainWorldPosition.y + normalizedHeight * mTerrainWorldScale.y;

    return worldY;
}



float TerrainComponent::GetHeightAtUV(float fullU, float fullV) const
{
    if (!mHeightmap)
        return 0.0f;

    shared_ptr<Texture> heightMap = mHeightmap->GetTexture(2);
    if (!heightMap)
        return 0.0f;


    float normalizedHeight = heightMap->GetHeightValue(fullU, fullV);


    return normalizedHeight *mTerrainWorldScale.y;
}
