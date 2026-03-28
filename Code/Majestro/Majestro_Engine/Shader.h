#pragma once
#include "Object.h"
#include "RootSignature.h"
#include "Entity.h"
class DescriptorHeap;
class Entity;

enum class SHADER_TYPE : uint8
{
	DEFERRED,
	FORWARD,
	LIGHTING,
	PARTICLE,
	COMPUTE,
	SHADOW,
	BILBOARD,
	DEPTH_PREPASS,
	BLUR,
	UI,
	TONEMAP,
	LDRPOST,
	COMPOSITE,

	END
};


enum class RASTERIZER_TYPE : uint8
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK,
	WIREFRAME,
};

enum class DEPTH_STENCIL_TYPE : uint8	//깊이 판별시 어떤 방식
{
	LESS,
	LESS_EQUAL,
	GREATER,
	GREATER_EQUAL,
	NO_DEPTH_TEST, // 깊이 테스트(X) + 깊이 기록(O)
	NO_DEPTH_TEST_NO_WRITE, // 깊이 테스트(X) + 깊이 기록(X)
	LESS_NO_WRITE, // 깊이 테스트(O) + 깊이 기록(X)
	EQUAL_NO_WRITE,
};

enum class BLEND_TYPE : uint8
{
	DEFAULT,
	ALPHA_BLEND,
	ONE_TO_ONE_BLEND,
	ALPHA_TEST,	// 알파 컷아웃 식생용: 블렌딩 없음, DepthPrePass 스킵
	END,
};


struct ShaderInfo
{
	SHADER_TYPE shaderType = SHADER_TYPE::FORWARD;
	RASTERIZER_TYPE rasterizerType = RASTERIZER_TYPE::CULL_BACK;
	DEPTH_STENCIL_TYPE depthStencilType = DEPTH_STENCIL_TYPE::LESS;
	BLEND_TYPE blendType = BLEND_TYPE::DEFAULT;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
};

struct ShaderPath {
	wstring VS = L"";
	wstring PS = L"";
	wstring CS = L"";
	wstring GS = L"";
	wstring HS = L"";
	wstring DS = L"";

};

struct ShaderArg
{
	const string vs = "VS_Main";
	const string hs;
	const string ds;
	const string gs;
	const string ps = "PS_Main";
};

class Shader : public Object
{
public:
	Shader();
	virtual ~Shader();
	void SetTargetFormat(DXGI_FORMAT format) { mShaderTargetFormat = format; }

	void CreateGraphicsShader(const ShaderPath& path, ShaderInfo info = ShaderInfo(),int msaaCount = 1 ,const string& vs = "VS_Main", const string& ps = "PS_Main", const string& gs = "");
	void CreateGraphicsShader(const ShaderPath& path, ShaderInfo info = ShaderInfo(),int msaaCount = 1, ShaderArg arg = ShaderArg());

	void CreateComputeShader(const ShaderPath& path, const string& name = "CS_Main", const string& version = "cs_5_1");

	void Update();


	
	BLEND_TYPE GetBlendType() { return mInfo.blendType; }
	SHADER_TYPE GetShaderType() { return mInfo.shaderType; }
	ComPtr<ID3D12PipelineState> GetPipelineState() { return mPipelineState; }

	static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(D3D_PRIMITIVE_TOPOLOGY topology);

private:
	void CreateShader(const wstring& path, const string& name, const string& version, ComPtr<ID3DBlob>& blob, D3D12_SHADER_BYTECODE& shaderByteCode);
	void CreateVertexShader(const wstring& path, const string& name, const string& version);
	void CreatePixelShader(const wstring& path, const string& name, const string& version);
	void CreateGeometryShader(const wstring& path, const string& name, const string& version);

	void CreateHullShader(const wstring& path, const string& name, const string& version);
	void CreateDomainShader(const wstring& path, const string& name, const string& version);

private:

	ShaderInfo		mInfo;
	DXGI_FORMAT		mShaderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;// DXGI_FORMAT_R16G16B16A16_FLOAT

	ComPtr<ID3D12PipelineState>			mPipelineState;	

	//GraphicsShader
	ComPtr<ID3DBlob>					mVsBlob;
	ComPtr<ID3DBlob>					mPsBlob;
	ComPtr<ID3DBlob>					mGsBlob;
	ComPtr<ID3DBlob>					mDsBlob;
	ComPtr<ID3DBlob>					mHsBlob;
	ComPtr<ID3DBlob>					mErrBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC  mGraphicsPipelineDesc = {};

	// ComputeShader
	ComPtr<ID3DBlob>					mCsBlob;
	D3D12_COMPUTE_PIPELINE_STATE_DESC   mComputePipelineDesc = {};

};

