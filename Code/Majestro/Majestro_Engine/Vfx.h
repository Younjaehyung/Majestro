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
};

