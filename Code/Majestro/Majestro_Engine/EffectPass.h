#pragma once
#include "World.h"
#include "RenderPass.h"
using EfkString = std::basic_string<EFK_CHAR>;
class Vfx;
class VfxComponent;
class Texture;
struct ID3D12Resource;

class EffectPass : public RenderPass
{
public:
	EffectPass() = default;
	~EffectPass();

	void Initialize(World* world);

	// 시뮬레이션 + HDR RT에 렌더링
	// viewMat, projMat: 카메라 행렬 (Effekseer 포맷)
	void Execute(float dt, const Effekseer::Matrix44& viewMat, const Effekseer::Matrix44& projMat, float zNear, float zFar);

	Effekseer::EffectRef LoadEffect(const std::string_view path, float magnification = 1.0f,
	                                const std::string_view materialPath = {});
	Effekseer::EffectRef LoadEffect(const shared_ptr<Vfx>& vfx);

	Effekseer::Handle Play(Entity owner, VfxComponent* comp, float x, float y, float z);
	Effekseer::Handle Play(Entity owner, VfxComponent* comp, const Effekseer::Vector3D& position);
	void Stop(Entity owner, VfxComponent* comp, bool allowRootStop = true);


	void MarkPlayFailed(VfxComponent* comp);		// 재생을 시작하지 못한 VFX를 끝난 것으로 표시

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

	inline Effekseer::Matrix43 ToEfkMatrix43(const Matrix& m)
	{
		Effekseer::Matrix43 out{};
		out.Value[0][0] = m._11; out.Value[0][1] = m._12; out.Value[0][2] = m._13;
		out.Value[1][0] = m._21; out.Value[1][1] = m._22; out.Value[1][2] = m._23;
		out.Value[2][0] = m._31; out.Value[2][1] = m._32; out.Value[2][2] = m._33;
		out.Value[3][0] = m._41; out.Value[3][1] = m._42; out.Value[3][2] = m._43;
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

	void* MakeOwnerToken(Entity owner)
	{
		return reinterpret_cast<void*>(static_cast<uintptr_t>(owner.GetID()));
	}

	bool IsInvalidHandle(Effekseer::Handle handle)
	{
		return handle == -1;
	}

	void UpdateSceneTextures(EffekseerRenderer::RendererRef renderer, const Effekseer::Matrix44& projMat);
	Effekseer::Backend::TextureRef GetOrCreateEfkTexture(shared_ptr<Texture> texture,
		ID3D12Resource*& cachedResource, Effekseer::Backend::TextureRef& cachedTexture);

	static float GetLoopCycleSeconds(const shared_ptr<Vfx>& vfx);

private:
	// Effekseer 매니저가 미리 확보하는 인스턴스 수.
	static constexpr int32 kInstanceMax = 8000;

	World* mWorld = nullptr;
	float mTotalTime = 0.0f;
	std::unordered_map<std::wstring, Effekseer::EffectRef> mEffectCache;
	Effekseer::RefPtr<Effekseer::Manager> mManager;
	Effekseer::RefPtr<Effekseer::Setting> mSetting;
	ID3D12Resource* mCachedDepthResource = nullptr;
	Effekseer::Backend::TextureRef mCachedDepthTexture;
	ID3D12Resource* mCachedBackgroundResource = nullptr;
	Effekseer::Backend::TextureRef mCachedBackgroundTexture;
};
