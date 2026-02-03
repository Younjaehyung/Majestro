#include "pch.h"
#include "TerrainComponent.h"
#include "Texture.h"


TerrainComponent::TerrainComponent(float width, float depth, shared_ptr<Material>& material)

{

	mHeightmap = material;
	shared_ptr<Texture> heightMap = material->GetTexture(2);
	mTerrainParams = {

		.TileCountX = (int)width,
		.TileCountZ = (int)depth,
		.MinMaxTessDistance = Vec2(100.f , 5000.f),


		.MaxTessLevel = 4.0f,
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

    const float tileCountX = static_cast<float>(mTerrainParams.TileCountX); // g_int_1
    const float tileCountZ = static_cast<float>(mTerrainParams.TileCountZ); // g_int_2

    Vec3 localPos = worldPos - mTerrainWorldPosition;

    if (mTerrainWorldScale.x != 0.0f)
        localPos.x /= mTerrainWorldScale.x;
    if (mTerrainWorldScale.z != 0.0f)
        localPos.z /= mTerrainWorldScale.z;


    float fullU = localPos.x / tileCountX;
    float fullV = 1.f - (localPos.z / tileCountZ);


    if (fullU < 0.0f || fullU > 1.0f ||
        fullV < 0.0f || fullV > 1.0f)
    {
        return 0.0f;
    }

    const float normalizedHeight = heightMap->GetHeightValue(fullU, fullV); // 0~1


    const float worldY =
        mTerrainWorldPosition.y + normalizedHeight * mTerrainWorldScale.y;

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
