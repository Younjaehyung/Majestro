#pragma once
#include "Object.h"


class Vfx : public Object
{
public:
	Vfx();
	~Vfx() = default;
	void Load(const std::wstring& path);

public:

	std::wstring mEffectPath;

	int32 mStartFrame = 0;		// 재생을 시작할 프레임
	int32 mLoopEndFrame = 0;	// 되감을 프레임
};

