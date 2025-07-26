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
};

enum class BLEND_TYPE : uint8
{
	DEFAULT,
	ALPHA_BLEND,
	ONE_TO_ONE_BLEND,
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

class Shader : public Object
{
public:
	Shader();
	virtual ~Shader();


	void CreateGraphicsShader(const wstring& path, ShaderInfo info = ShaderInfo(), const string& vs = "VS_Main", const string& ps = "PS_Main", const string& gs = "");
	//void CreateComputeShader(const wstring& path, const string& name, const string& version);

	void Update();

	SHADER_TYPE GetShaderType() { return _info.shaderType; }

	shared_ptr< RootSignature>			GetRootSignature() { return mRootSignature; }

	void PushBackEntity(Entity entityID) { mEntity.push_back(entityID); }

	static D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType(D3D_PRIMITIVE_TOPOLOGY topology);
	

private:
	void CreateShader(const wstring& path, const string& name, const string& version, ComPtr<ID3DBlob>& blob, D3D12_SHADER_BYTECODE& shaderByteCode);
	void CreateVertexShader(const wstring& path, const string& name, const string& version);
	void CreatePixelShader(const wstring& path, const string& name, const string& version);
	void CreateGeometryShader(const wstring& path, const string& name, const string& version);



private:

	shared_ptr<RootSignature>			mRootSignature = make_shared<RootSignature>();
	ShaderInfo _info;
	ComPtr<ID3D12PipelineState>			mPipelineState;
	

	//GraphicsShader
	ComPtr<ID3DBlob>					mVsBlob;
	ComPtr<ID3DBlob>					mPsBlob;
	ComPtr<ID3DBlob>					mGsBlob;
	ComPtr<ID3DBlob>					mErrBlob;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC  mGraphicsPipelineDesc = {};

	// ComputeShader
	ComPtr<ID3DBlob>					_csBlob;
	D3D12_COMPUTE_PIPELINE_STATE_DESC   _computePipelineDesc = {};


	vector<Entity> mEntity;
};

