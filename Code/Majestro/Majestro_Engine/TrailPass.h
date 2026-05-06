#pragma once
#include "World.h"

class Shader;

class TrailPass
{
public:
	TrailPass() = default;
	~TrailPass() = default;

	void Initialize(World* world);
	void Execute();

private:
	void CreateShader();
	void CreateBuffers();
	static void FillTrailVertex(Vertex& outVertex, const Vec3& worldPos, const Vec2& uv, const Vec4& color,
		int32 textureIndex, float textureAlphaWeight, bool useTextureColor);

private:
	static constexpr uint32 MAX_TRAIL_VERTICES = 8192;
	static constexpr uint32 MAX_TRAIL_INDICES = 24576;

	World* mWorld = nullptr;
	shared_ptr<Shader> mShader;

	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mIndexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVBV = {};
	D3D12_INDEX_BUFFER_VIEW mIBV = {};

	Vertex* mMappedVertices = nullptr;
	uint16* mMappedIndices = nullptr;
};
