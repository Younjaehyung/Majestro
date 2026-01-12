#include "pch.h"
#include "HeightField.h"

#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"

HeightField::HeightField() : Object(OBJECT_TYPE::HEIGHTFIELD)
{
}

HeightField::~HeightField()
{
}

void HeightField::LoadHeightFieldFromPng16(const std::string& pngPath)
{
    int w = 0, h = 0, comp = 0;

    uint16_t* pixels = stbi_load_16(pngPath.c_str(), &w, &h, &comp, 0);
    if (!pixels)
        throw std::runtime_error("PNG 로드 실패: " + pngPath);

	mPath = pngPath;
    mWidth = w;
    mHeight = h;
   /* mOriginX = originX;
    mOriginZ = originZ;
    mWorldSizeX = worldSizeX;
    mWorldSizeZ = worldSizeZ;
    mMinY = minY;
    mMaxY = maxY;*/
    mSamples.resize((size_t)w * h);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            size_t idx = (size_t)y * w + x;
            size_t p = idx * (size_t)comp;

            uint16_t heightU16 = 0;

            if (comp == 1 || comp == 2)
            {

                heightU16 = pixels[p + 0];
            }
            else
            {

                heightU16 = pixels[p + 0]; // R
            }

            mSamples[idx] = heightU16;
        }
    }

    stbi_image_free(pixels);

    // 필요 시 hf.h를 y축으로 flip 하는 코드를 추가

    //return hf;
}

float HeightField::GetHeightValue(float u, float v) const
{
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    float fx = u * (mWidth - 1);
    float fy = v * (mHeight - 1);
    uint32 x = static_cast<uint32>(fx);
    uint32 y = static_cast<uint32>(fy);
    return GetHeightValuePixel(x, y);
}

float HeightField::GetHeightValuePixel(float x, float y) const
{
    x = std::clamp(x, (float)0, static_cast<float>(mWidth - 1));
    y = std::clamp(y, (float)0, static_cast<float>(mHeight - 1));
    size_t index = static_cast<size_t>(y) * mWidth + static_cast<size_t>(x);
    if (index >= mSamples.size())
        return 0.0f;
    uint16_t heightU16 = mSamples[index];
    // 정규화된 높이 값으로 변환 (0.0 ~ 1.0)
    return static_cast<float>(heightU16) / 65535.0f;
}

//float HeightField::GetHeightValue(float u, float v) const
//{
//    u = std::clamp(u, 0.0f, 1.0f);
//    v = std::clamp(v, 0.0f, 1.0f);
//
//    float fx = u * (mWidth - 1);
//    float fy = v * (mHeight - 1);
//
//    uint32 x0 = static_cast<uint32>(fx);
//    uint32 y0 = static_cast<uint32>(fy);
//    uint32 x1 = min(x0 + 1, static_cast<uint32>(mWidth - 1));
//    uint32 y1 = min(y0 + 1, static_cast<uint32>(mHeight - 1));
//
//    float tx = fx - x0;
//    float ty = fy - y0;
//
//
//    float h00 = GetHeightValuePixel(x0, y0);
//    float h10 = GetHeightValuePixel(x1, y0);
//    float h01 = GetHeightValuePixel(x0, y1);
//    float h11 = GetHeightValuePixel(x1, y1);
//
//    float h0 = std::lerp(h00, h10, tx);
//    float h1 = std::lerp(h01, h11, tx);
//    return std::lerp(h0, h1, ty);
//}
//
//float HeightField::GetHeightValuePixel(float u, float v) const
//{
//    u = std::clamp(u, 0.0f, 1.0f);
//    v = std::clamp(v, 0.0f, 1.0f);
//    float fx = u * (mWidth - 1);
//    float fy = v * (mHeight - 1);
//    uint32 x = static_cast<uint32>(fx);
//    uint32 y = static_cast<uint32>(fy);
//    return GetHeightValuePixel(x, y);
//}