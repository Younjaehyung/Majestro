#include "pch.h"
#include "Vfx.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

Vfx::Vfx() : Object(OBJECT_TYPE::VFX)
{
	mEffect = nullptr;
}

void Vfx::Load(const std::wstring& path)
{
	mEffectPath = path;
}