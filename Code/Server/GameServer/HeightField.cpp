#include "pch.h"
#include "HeightField.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"

HeightField::HeightField() : Object(OBJECT_TYPE::HEIGHTFIELD) {}

HeightField::~HeightField() {}

void HeightField::Load(const wstring &path) {
  std::filesystem::path filePath(path);
  std::string extension = filePath.extension().string();
  std::transform(
      extension.begin(), extension.end(), extension.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

  if (extension == ".png") {
    LoadHeightFieldFromPng16(filePath.string());
    return;
  }

  if (extension == ".raw") {
    throw std::runtime_error("RAW 로드는 크기 정보가 필요합니다: " +
                             filePath.string());
  }

  throw std::runtime_error("지원하지 않는 HeightField 형식: " +
                           filePath.string());
}

void HeightField::LoadHeightFieldFromPng16(const std::string &pngPath) {
  int w = 0, h = 0, comp = 0;

  uint16_t *pixels = stbi_load_16(pngPath.c_str(), &w, &h, &comp, 0);
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

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      size_t idx = (size_t)y * w + x;
      size_t p = idx * (size_t)comp;

      uint16_t heightU16 = 0;

      if (comp == 1 || comp == 2) {

        heightU16 = pixels[p + 0];
      } else {

        heightU16 = pixels[p + 0]; // R
      }

      mSamples[idx] = heightU16;
    }
  }

  stbi_image_free(pixels);

  // 필요 시 hf.h를 y축으로 flip 하는 코드를 추가

  // return hf;
}

void HeightField::LoadHeightFieldFromRaw16(const std::string &rawPath,
                                           int width, int height,
                                           bool littleEndian) {
  if (width <= 0 || height <= 0)
    throw std::runtime_error("RAW 로드 실패: 유효하지 않은 크기");

  std::ifstream file(rawPath, std::ios::binary);
  if (!file)
    throw std::runtime_error("RAW 로드 실패: " + rawPath);

  const size_t sampleCount =
      static_cast<size_t>(width) * static_cast<size_t>(height);
  const size_t expectedBytes = sampleCount * sizeof(uint16_t);

  std::vector<uint8_t> buffer(expectedBytes);
  file.read(reinterpret_cast<char *>(buffer.data()), expectedBytes);
  if (static_cast<size_t>(file.gcount()) != expectedBytes)
    throw std::runtime_error("RAW 로드 실패: " + rawPath +
                             "에서 예상된 바이트 수를 읽지 못함");

  mPath = rawPath;
  mWidth = width;
  mHeight = height;
  mSamples.resize(sampleCount);

  for (size_t i = 0; i < sampleCount; ++i) {
    uint16_t value = static_cast<uint16_t>(buffer[i * 2]) |
                     (static_cast<uint16_t>(buffer[i * 2 + 1]) << 8);

    if (!littleEndian)
      value = static_cast<uint16_t>((value >> 8) | (value << 8));

    mSamples[i] = value;
  }

  // FlipVertically();
}

void HeightField::FlipVertically() {
  std::vector<uint16_t> tempSamples(mSamples.size());

  for (int y = 0; y < mHeight; ++y) {
    for (int x = 0; x < mWidth; ++x) {
      // 원본 : (x, y)
      // 타겟 : (x, height - 1 - y)
      int originalIndex = y * mWidth + x;
      int targetIndex = (mHeight - 1 - y) * mWidth + x;

      tempSamples[targetIndex] = mSamples[originalIndex];
    }
  }

  mSamples = std::move(tempSamples);
}

float HeightField::GetHeightValue(float u, float v) const {
  u = std::clamp(u, 0.0f, 1.0f);
  v = std::clamp(v, 0.0f, 1.0f);
  float fx = u * (mWidth - 1);
  float fy = v * (mHeight - 1);
  uint32 x0 = static_cast<uint32>(fx);
  uint32 y0 = static_cast<uint32>(fy);
  uint32 x1 = min(x0 + 1, static_cast<uint32>(mWidth - 1));
  uint32 y1 = min(y0 + 1, static_cast<uint32>(mHeight - 1));

  float tx = fx - x0;
  float ty = fy - y0;

  float h00 = GetHeightValuePixel(x0, y0);
  float h10 = GetHeightValuePixel(x1, y0);
  float h01 = GetHeightValuePixel(x0, y1);
  float h11 = GetHeightValuePixel(x1, y1);

  float h0 = std::lerp(h00, h10, tx);
  float h1 = std::lerp(h01, h11, tx);
  return std::lerp(h0, h1, ty);
}

float HeightField::GetHeightValuePixel(float x, float y) const {
  if (x < 0.0f || y < 0.0f)
    return 0.0f;

  if (x >= static_cast<float>(mWidth) || y >= static_cast<float>(mHeight))
    return 0.0f;

  size_t index = static_cast<size_t>(y) * mWidth + static_cast<size_t>(x);
  uint16_t heightU16 = mSamples[index];
  // 정규화된 높이 값으로 변환 (0.0 ~ 1.0)
  return static_cast<float>(heightU16) / 65535.0f;
}

// float HeightField::GetHeightValue(float u, float v) const
//{
//     u = std::clamp(u, 0.0f, 1.0f);
//     v = std::clamp(v, 0.0f, 1.0f);
//
//     float fx = u * (mWidth - 1);
//     float fy = v * (mHeight - 1);
//
//     uint32 x0 = static_cast<uint32>(fx);
//     uint32 y0 = static_cast<uint32>(fy);
//     uint32 x1 = min(x0 + 1, static_cast<uint32>(mWidth - 1));
//     uint32 y1 = min(y0 + 1, static_cast<uint32>(mHeight - 1));
//
//     float tx = fx - x0;
//     float ty = fy - y0;
//
//
//     float h00 = GetHeightValuePixel(x0, y0);
//     float h10 = GetHeightValuePixel(x1, y0);
//     float h01 = GetHeightValuePixel(x0, y1);
//     float h11 = GetHeightValuePixel(x1, y1);
//
//     float h0 = std::lerp(h00, h10, tx);
//     float h1 = std::lerp(h01, h11, tx);
//     return std::lerp(h0, h1, ty);
// }
//
// float HeightField::GetHeightValuePixel(float u, float v) const
//{
//     u = std::clamp(u, 0.0f, 1.0f);
//     v = std::clamp(v, 0.0f, 1.0f);
//     float fx = u * (mWidth - 1);
//     float fy = v * (mHeight - 1);
//     uint32 x = static_cast<uint32>(fx);
//     uint32 y = static_cast<uint32>(fy);
//     return GetHeightValuePixel(x, y);
// }