#pragma once
#include "World.h"

using EfkString = std::basic_string<EFK_CHAR>;
class VfxComponent;

class EffectPass
{
public:
	EffectPass() = default;
	~EffectPass();

	void Initialize(World* world);

	// 시뮬레이션 + HDR RT에 렌더링
	// viewMat, projMat: 카메라 행렬 (Effekseer 포맷)
	void Execute(float dt, const Effekseer::Matrix44& viewMat, const Effekseer::Matrix44& projMat);

	Effekseer::EffectRef LoadEffect(const std::string_view path, float magnification = 1.0f,
	                                const std::string_view materialPath = {});

	Effekseer::Handle Play(VfxComponent* comp, float x, float y, float z);
	Effekseer::Handle Play(VfxComponent* comp, const Effekseer::Vector3D& position);

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
};
