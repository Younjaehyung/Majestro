#pragma once

#include "Device.h"
#include "CommandQueue.h"
#include "RootSignature.h"
#include "DescriptorHeap.h"
#include "Buffer.h"
#include "SwapChain.h"
#include "RenderTarget.h"

class SceneManager;


enum class LIGHT_TYPE : uint8
{
	DIRECTIONAL_LIGHT,
	POINT_LIGHT,
	SPOT_LIGHT,
};

struct LightColor	//빛의 3개 속성
{
	Vec4	diffuse;
	Vec4	ambient;
	Vec4	specular;
};

struct LightInfo	//빛과 관련된 정보
{
	LightColor	color;
	Vec4		position;	//DIRECTIONAL_LIGHT은 사실상 필요 없음
	Vec4		direction;	//POINT_LIGHT은 사실상 필요 없음
	int32		lightType;	//LIGHT_TYPE
	float		range;
	float		angle;
	int32		padding;	//데이터 사이즈용 padding
};

struct LightParams
{
	uint32		lightCount;
	Vec3		padding;	//데이터 사이즈용 padding
	LightInfo	lights[50];
};

struct CameraParams {
	Matrix MatView;
	Matrix MatProjection;
	Matrix MatViewInv;				// view의 역행렬
	Matrix MatProjectionInv;		// Projection의 역행렬	(사용은 선택)
};

class RenderManager
{
public:
	void Initialize(const WindowInfo& info);
	void Update();

	void StartRender();
	void Render();
	void EndRender();

	void ResizeWindow(int32 width, int32 height);

	

	const WindowInfo& GetWindow() { return mWindow; }
public:


	ID3D12DescriptorHeap* GetLegacyGraphicsDescriptorHeap() { return mGraphicsDescHeap->GetDescriptorHeap().Get(); }

public:
	shared_ptr<Device>					GetDevice()				{ return mDevice; }
	shared_ptr<GraphicsCommandQueue>	GetGraphicsCmdQueue()	{ return mGraphicsCommandQueue; }
	//shared_ptr< ComputeCommandQueue>	GetComputeCmdQueue()	{ return mComputeCmdQueue; }

	shared_ptr<SwapChain>				GetSwapChain()			{ return mSwapChain; }
	
	shared_ptr<GraphicsDescriptorHeap> GetGraphicsDescHeap()	{ return mGraphicsDescHeap; }
	//shared_ptr< ComputeDescriptorHeap>	GetComputeDescHeap()	{ return mComputeDescHeap; }


	shared_ptr<ConstantBuffer> GetConstantBuffer(CONSTANT_BUFFER_TYPE type) { return mConstantBuffer[static_cast<uint8>(type)]; }
	vector<shared_ptr<ConstantBuffer>>& GetConstantBuffers() { return mConstantBuffer; }
	shared_ptr<RenderTarget> GetRTGroup(RENDER_TARGET_GROUP_TYPE type) { return mRenderTargetGroups[static_cast<uint8>(type)]; }

private:
	void CreateRenderTargetGroups();
	void CreateConstantBuffer(CBV_REGISTER reg, uint32 bufferSize, uint32 count);	//버퍼 생성

private:
	// buffer
	ComPtr<ID3D12Resource>	LIghtBuffer;
	ComPtr<ID3D12Resource>	ObjectBuffer;
	ComPtr<ID3D12Resource>	MaterialBuffer;

private:
	shared_ptr<Device>					mDevice						= make_shared<Device>();
	shared_ptr<GraphicsCommandQueue>	mGraphicsCommandQueue		= make_shared<GraphicsCommandQueue>();
	
	shared_ptr<SwapChain>				mSwapChain					= make_shared<SwapChain>();
	shared_ptr<GraphicsDescriptorHeap>	mGraphicsDescHeap			= make_shared<GraphicsDescriptorHeap>();
	vector<shared_ptr<ConstantBuffer>>		mConstantBuffer;

	//shared_ptr<ComputeDescriptorHeap> _computeDescHeap = make_shared<ComputeDescriptorHeap>();
	//shared_ptr<ComputeCommandQueue>		mComputeCmdQueue	= make_shared<ComputeCommandQueue>();

	array<shared_ptr<RenderTarget>, RENDER_TARGET_GROUP_COUNT> mRenderTargetGroups;
private:

	WindowInfo		mWindow;
	D3D12_VIEWPORT	mViewport{};
	D3D12_RECT		mScissorRect{};
};

