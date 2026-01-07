#pragma once
#include "System.h"
#include "World.h"
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>
#include <LLGI.Platform.h>

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
        int32_t squareMaxCount = 2000)
    {
        // 1) Backend GraphicsDevice
        graphicsDevice_ = EffekseerRendererDX12::CreateGraphicsDevice(device, commandQueue, swapBufferCount);
        if (graphicsDevice_ == nullptr) return false; // CreateGraphicsDevice :contentReference[oaicite:8]{index=8}

        // 2) Renderer
        DXGI_FORMAT rtFormats[1] = { rtvFormat };
        renderer_ = EffekseerRendererDX12::Create(
            graphicsDevice_,
            rtFormats,
            1,
            dsvFormat,
            isReversedDepth,
            squareMaxCount);
        //if (renderer_ == nullptr) return false; // Create :contentReference[oaicite:9]{index=9}

        // 3) Manager
        manager_ = Effekseer::Manager::Create(instanceMax);
        //if (manager_ == nullptr) return false; // Manager::Create :contentReference[oaicite:10]{index=10}

        // (권장) 4) Setting + Loader 세팅 (효과 파일/텍스처/모델/머티리얼 로딩 경로 제어에 유리)
        setting_ = Effekseer::Setting::Create();
        // setting_->SetCoordinateSystem(...) 같은 설정은 "이펙트 로드 전"에 하는게 안전합니다. :contentReference[oaicite:11]{index=11}

        memoryPool_ = EffekseerRenderer::CreateSingleFrameMemoryPool(graphicsDevice_);
        commandList_ = EffekseerRenderer::CreateCommandList(graphicsDevice_, memoryPool_);


        // Sprcify rendering modules
    // 描画モジュールの設定
        manager_->SetSpriteRenderer(renderer_->CreateSpriteRenderer());
        manager_->SetRibbonRenderer(renderer_->CreateRibbonRenderer());
        manager_->SetRingRenderer(renderer_->CreateRingRenderer());
        manager_->SetTrackRenderer(renderer_->CreateTrackRenderer());
        manager_->SetModelRenderer(renderer_->CreateModelRenderer());

        // Specify a texture, model, curve and material loader
        // It can be extended by yourself. It is loaded from a file on now.
        // テクスチャ、モデル、カーブ、マテリアルローダーの設定する。
        // ユーザーが独自で拡張できる。現在はファイルから読み込んでいる。
        manager_->SetTextureLoader(renderer_->CreateTextureLoader());
        manager_->SetModelLoader(renderer_->CreateModelLoader());
        manager_->SetMaterialLoader(renderer_->CreateMaterialLoader());
        manager_->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

        return (memoryPool_ != nullptr && commandList_ != nullptr);
    }

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
    Effekseer::EffectRef LoadEffect(const EFK_CHAR* path, float magnification = 1.0f, const EFK_CHAR* materialPath = nullptr)
    {
        // Effect::Create(manager, path, magnification, materialPath) :contentReference[oaicite:14]{index=14}
		return Effekseer::Effect::Create(manager_,path,magnification, materialPath);
    }

    // 재생
    Effekseer::Handle Play(Effekseer::EffectRef& effect, float x, float y, float z);

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
        // 아래 out.Values[][] 는 Effekseer 1.7에서 일반적으로 쓰이는 멤버명이다.
        // 만약 네 Effekseer 헤더에서 멤버명이 다르면(예: out.Value, out.Values),
        // 그 이름에 맞게 여기만 바꾸면 된다.
        out.Values[0][0] = f._11; out.Values[0][1] = f._12; out.Values[0][2] = f._13; out.Values[0][3] = f._14;
        out.Values[1][0] = f._21; out.Values[1][1] = f._22; out.Values[1][2] = f._23; out.Values[1][3] = f._24;
        out.Values[2][0] = f._31; out.Values[2][1] = f._32; out.Values[2][2] = f._33; out.Values[2][3] = f._34;
        out.Values[3][0] = f._41; out.Values[3][1] = f._42; out.Values[3][2] = f._43; out.Values[3][3] = f._44;
        return out;
    }

    // (선택) 벡터 변환도 샘플에서 같이 쓰는 경우가 많아서 같이 제공
    inline Effekseer::Vector3D ToEfkVector3(const DirectX::XMFLOAT3& v)
    {
        return Effekseer::Vector3D(v.x, v.y, v.z);
    }
private:
    Effekseer::RefPtr<Effekseer::Manager> manager_;
    Effekseer::RefPtr<Effekseer::Setting> setting_;

    Effekseer::Backend::GraphicsDeviceRef graphicsDevice_;
    EffekseerRenderer::RendererRef renderer_;

    Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> memoryPool_;
    Effekseer::RefPtr<EffekseerRenderer::CommandList> commandList_;
    Effekseer::Handle efkHandle = 0;
    std::shared_ptr<LLGI::Platform> platform;
    std::shared_ptr<LLGI::Graphics> graphics;
	Effekseer::EffectRef effect_ = nullptr;
};

