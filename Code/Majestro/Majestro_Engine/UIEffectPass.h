#pragma once
#include "World.h"

using EfkString = std::basic_string<EFK_CHAR>;
class UIVfxComponent;

class UIEffectPass
{
public:
	UIEffectPass() = default;
	~UIEffectPass();

	void Initialize(World* world);

	// 시뮬레이션 + SwapChain RT에 렌더링 (UI 위에 표시)
	void Execute(float dt);

	Effekseer::EffectRef LoadEffect(const std::string_view path, float magnification = 1.0f);
	Effekseer::Handle    Play(UIVfxComponent* comp);

	void LoadResources();

	inline Effekseer::Matrix44 ToEfkString_unused() { return {}; } // 미사용 방지용 placeholder

private:
	// 픽셀 좌표 (0,0)~(W,H) → NDC (-1,1) 직교 투영 행렬
	Effekseer::Matrix44 BuildOrthoProjection();

	inline EfkString ToEfkString(std::string_view utf8)
	{
		return WideToEfk(Utf8ToWide(utf8));
	}

	inline std::wstring Utf8ToWide(std::string_view utf8)
	{
		if (utf8.empty()) return {};
		int required = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
		if (required <= 0) return {};
		std::wstring wide(required, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), wide.data(), required);
		return wide;
	}

	inline EfkString WideToEfk(std::wstring_view wide)
	{
		if (wide.empty()) return {};
		if constexpr (std::is_same_v<EFK_CHAR, wchar_t>)
			return EfkString(wide.begin(), wide.end());
		else if constexpr (std::is_same_v<EFK_CHAR, char16_t>)
		{
			EfkString out; out.resize(wide.size());
			for (size_t i = 0; i < wide.size(); ++i) out[i] = static_cast<char16_t>(wide[i]);
			return out;
		}
		return {};
	}

private:
	World* mWorld = nullptr;
	Effekseer::RefPtr<Effekseer::Manager> mManager;
	Effekseer::RefPtr<Effekseer::Setting> mSetting;
	float mTotalTime = 0.f;
};
