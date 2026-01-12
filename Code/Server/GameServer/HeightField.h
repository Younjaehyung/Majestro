#pragma once
#include "Object.h"
#include <vector>

class HeightField : public Object
{
public:
	HeightField();
	~HeightField();

	void LoadHeightFieldFromPng16(const std::string& path);

	float GetHeightAtWorldPosition(const Vec3& worldPos) const;
	float GetHeightValue(float u, float v) const;
	float GetHeightValuePixel(float u, float v) const;

public:

	// 샘플의 크기 (해상도)
	int mWidth = 0;
	int mHeight = 0;



	// 0~65535	
	std::vector<uint16_t> mSamples;
	std::string mPath;

	friend class ResouceManager;
};

