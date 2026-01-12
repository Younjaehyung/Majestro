#include "pch.h"
#include "TerrainComponent.h"
#include "HeightField.h"

TerrainComponent::TerrainComponent(float width, float depth, shared_ptr<HeightField> heightField)
{

	mHeightField = heightField;

    Vec2 heightRes = Vec2(0, 0);
    if (mHeightField)
        heightRes = Vec2((float)mHeightField->mWidth, (float)mHeightField->mHeight);

    mTerrainParams =
    {
        .TileCountX = (int)width,
        .TileCountZ = (int)depth,
        .MinMaxTessDistance = Vec2(1000.f , 5000.f),
        .MaxTessLevel = 4.0f,
        .HeightMapResolution = heightRes
    };
	
}

float TerrainComponent::GetHeightAtWorldPosition(const Vec3& worldPos) const
{
    if (!mHeightField)
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

    const float normalizedHeight = mHeightField->GetHeightValue(fullU, fullV); // 0~1


    const float worldY =
        mTerrainWorldPosition.y + normalizedHeight * mTerrainWorldScale.y;

    return worldY;
}

float TerrainComponent::GetHeightAtUV(float fullU, float fullV) const
{
    if (!mHeightField)
        return 0.0f;


    float normalizedHeight = mHeightField->GetHeightValue(fullU, fullV);


    return normalizedHeight *mTerrainWorldScale.y;
}
