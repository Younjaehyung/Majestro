#pragma once
#include "System.h"
#include "World.h"

using EfkString = std::basic_string<EFK_CHAR>;
class Vfx;
class UIVfxComponent;

// UI 레이어 전용 Effekseer 시스템
// UIRenderSystem(order=2) 이후에 실행되어 스프라이트 위에 VFX를 렌더링
class UIEffectSystem : public System
{
public:
	UIEffectSystem(World* world);
	~UIEffectSystem() override;

	void Initialize();
	void Update(float deltaTimeSeconds);
	void Update();

	// 이펙트 로드 (EffectSystem과 독립적으로 로드)
	Effekseer::EffectRef LoadEffect(const std::string_view path, float magnification = 1.0f);
	Effekseer::EffectRef LoadEffect(const shared_ptr<Vfx>& vfx);

	// UITransformComponent.mFinalPixelPos 기준으로 재생
	Effekseer::Handle Play(UIVfxComponent* comp, float screenX, float screenY);

	void LoadResources();

	inline Effekseer::Matrix44 ToEfkMatrix(const DirectX::XMMATRIX& m)
	{
		DirectX::XMFLOAT4X4 f{};
		DirectX::XMStoreFloat4x4(&f, m);
		Effekseer::Matrix44 out{};
		out.Values[0][0] = f._11; out.Values[0][1] = f._12; out.Values[0][2] = f._13; out.Values[0][3] = f._14;
		out.Values[1][0] = f._21; out.Values[1][1] = f._22; out.Values[1][2] = f._23; out.Values[1][3] = f._24;
		out.Values[2][0] = f._31; out.Values[2][1] = f._32; out.Values[2][2] = f._33; out.Values[2][3] = f._34;
		out.Values[3][0] = f._41; out.Values[3][1] = f._42; out.Values[3][2] = f._43; out.Values[3][3] = f._44;
		return out;
	}

	inline EfkString ToEfkString(std::string_view utf8)
	{
		return WideToEfk(Utf8ToWide(utf8));
	}

private:
	// 스크린 픽셀 좌표 → NDC 직교 투영 행렬
	// (0,0)=좌상단, (W,H)=우하단 기준
	Effekseer::Matrix44 BuildOrthoProjection();

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
	std::unordered_map<std::wstring, Effekseer::EffectRef> mEffectCache;
	Effekseer::RefPtr<Effekseer::Manager> uiManager_;
	Effekseer::RefPtr<Effekseer::Setting> setting_;
	float mTotalTime = 0.f;
};
