#include "pch.h"
#include "Vfx.h"

Vfx::Vfx() : Object(OBJECT_TYPE::VFX)
{
}

void Vfx::Load(const std::wstring& path)
{
	mEffectPath = path;
}
