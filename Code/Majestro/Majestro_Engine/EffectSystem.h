#pragma once
#include "System.h"
#include "World.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>
#include <LLGI.Platform.h>

using EfkString = std::basic_string<EFK_CHAR>;

class EffectSystem : public System
{

public:
	EffectSystem(World* world);
	~EffectSystem() override;


	void Initialize();
    void Update(float deltaTimeSeconds);
	void Update();
private:
	//Effekseer::ManagerRef gEffekseerManager;
	//EffekseerRenderer::RendererRef gEffekseerRenderer;
public:
    bool Initialize(
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        DXGI_FORMAT rtvFormat,
        DXGI_FORMAT dsvFormat,
        int32_t swapBufferCount,
        bool isReversedDepth,
        int32_t instanceMax = 8000,
        int32_t squareMaxCount = 2000);

	void LoadResources();

    void Shutdown()
    {
        // RefPtr 기반이면 보통 nullptr 대입으로 정리됩니다.
        commandList_ = nullptr;
        memoryPool_ = nullptr;
        manager_ = nullptr;
        renderer_ = nullptr;
        graphicsDevice_ = nullptr;
        setting_ = nullptr;
    }

    // 이펙트 로드
    Effekseer::EffectRef LoadEffect(const EFK_CHAR* path, float magnification = 1.0f, const EFK_CHAR* materialPath = nullptr);
    Effekseer::EffectRef LoadEffect(const std::string_view path, float magnification = 1.0f, const std::string_view materialPath = {});
    // 재생
    Effekseer::Handle Play(Effekseer::EffectRef& effect, float x, float y, float z);
	Effekseer::Handle Play(Effekseer::EffectRef& effect, const Effekseer::Vector3D& position);

    // 프레임 시작: 엔진 커맨드리스트를 Effekseer에 연결
    void BeginFrame(ID3D12GraphicsCommandList* dxCmdList);

    // 렌더
    void Render(const Effekseer::Matrix44& camera, const Effekseer::Matrix44& projection);

    // 프레임 종료: Effekseer 커맨드리스트 마감/실행
    void EndFrame();

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

    inline Effekseer::Vector3D ToEfkVector3(const DirectX::XMFLOAT3& v)
    {
        return Effekseer::Vector3D(v.x, v.y, v.z);
    }
private:
    Effekseer::Handle efkHandle = 0;
    Effekseer::RefPtr<Effekseer::Manager> manager_;
    Effekseer::RefPtr<Effekseer::Setting> setting_;

   
    EffekseerRenderer::RendererRef renderer_;

    Effekseer::Backend::GraphicsDeviceRef graphicsDevice_;
    Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> memoryPool_;
    Effekseer::RefPtr<EffekseerRenderer::CommandList> commandList_;


public:
    // 수정사항: UTF-8(std::string) -> UTF-16(std::wstring) 변환 (Windows API 사용)
    inline std::wstring Utf8ToWide(std::string_view utf8)
    {
        if (utf8.empty())
            return {};

        int required = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
        if (required <= 0)
            return {};

        std::wstring wide(required, L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), wide.data(), required);
        return wide;
    }

    // 수정사항: std::wstring -> EfkString(EFK_CHAR 기반) 변환
    inline EfkString WideToEfk(std::wstring_view wide)
    {
        if (wide.empty())
            return {};

        if constexpr (std::is_same_v<EFK_CHAR, wchar_t>)
        {
            // EFK_CHAR == wchar_t 인 빌드
            return EfkString(wide.begin(), wide.end());
        }
        else if constexpr (std::is_same_v<EFK_CHAR, char16_t>)
        {
            // EFK_CHAR == char16_t 인 빌드 (Windows wchar_t는 UTF-16 16bit이므로 code unit 단위 복사)
            EfkString out;
            out.resize(wide.size());
            for (size_t i = 0; i < wide.size(); ++i)
                out[i] = static_cast<char16_t>(wide[i]);
            return out;
        }
        else if constexpr (std::is_same_v<EFK_CHAR, char>)
        {
            // EFK_CHAR == char 인 빌드 (이 경우 보통 UTF-8을 기대할 수 있으나, Effekseer 빌드 설정에 따라 다를 수 있음)
            // 보수적으로: wide -> UTF-8로 변환 후 반환
            int required = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
            if (required <= 0) return {};
            std::string utf8(required, '\0');
            ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), utf8.data(), required, nullptr, nullptr);
            return EfkString(utf8.begin(), utf8.end());
        }
        else
        {
            //static_assert(!sizeof(EFK_CHAR), "Unsupported EFK_CHAR type");
        }
    }

    // 수정사항: std::string(UTF-8) -> EfkString 변환
    inline EfkString ToEfkString(std::string_view utf8)
    {
        return WideToEfk(Utf8ToWide(utf8));
    }

    // 수정사항: std::wstring -> EfkString 변환
    inline EfkString ToEfkString(std::wstring_view wide)
    {
        return WideToEfk(wide);
    }

    // 수정사항: std::filesystem::path -> EfkString 변환 (Windows에서는 wstring 기반이 견고)
    inline EfkString ToEfkString(const std::filesystem::path& p)
    {
        return WideToEfk(p.wstring());
    }
};

