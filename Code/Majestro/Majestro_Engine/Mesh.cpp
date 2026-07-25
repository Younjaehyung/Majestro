#include "pch.h"
#include "Mesh.h"
#include "Engine.h"
#include "RenderManager.h"
#include "Material.h"
#include "FBXData.h"
#include "GpuResourceBudget.h"



Mesh::Mesh() : Object(OBJECT_TYPE::MESH)
{
}

Mesh::~Mesh()
{
}

void Mesh::Init(const vector<Vertex>& vec, const vector<uint32>& indexbuffer)
{
	BuildLocalOBBFromVertices(vec, mLocalOBB);
	CreateVertexBuffer(vec);
	CreateIndexBuffer(indexbuffer);
}



void Mesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	mVertexCount = static_cast<uint32>(buffer.size());
	const uint64 bufferSize64 = static_cast<uint64>(mVertexCount) * sizeof(Vertex);
	if (buffer.empty() || bufferSize64 > UINT_MAX)
	{
		mVertexBuffer.Reset();
		mVertexBufferView = {};
		assert(false);
		return;
	}
	const uint32 bufferSize = static_cast<uint32>(bufferSize64);

	std::wstring meshName = GetName().empty() ? L"UnnamedMesh" : GetName();
	const std::wstring resourceName = meshName + L" Vertex";

	// 정적 정점 데이터 기본 힙 복사
	const bool created = RENDERMANAGER.GetGraphicsCmdQueue()->CreateStaticBuffer(
		buffer.data(),
		bufferSize,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		mVertexBuffer,
		resourceName.c_str());

	if (!created)
	{
		mVertexBuffer.Reset();
		mVertexBufferView = {};
		assert(false);
		return;
	}

	GpuResourceBudget::RecordResource(DEVICE.Get(), L"MeshVertexDefault",
		resourceName + L" " + std::to_wstring(mVertexCount),
		D3D12_HEAP_TYPE_DEFAULT, mVertexBuffer.Get());

	// 리소스(데이터) 정보(속성) 설정
	// Initialize the vertex buffer view.
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();	// GPU VRAM의 주소를 지정
	mVertexBufferView.StrideInBytes = sizeof(Vertex); // 정점 1개 크기
	mVertexBufferView.SizeInBytes = bufferSize; // 버퍼의 크기	
}

void Mesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	uint32 indexCount = static_cast<uint32>(buffer.size());
	const uint64 bufferSize64 = static_cast<uint64>(indexCount) * sizeof(uint32);
	if (buffer.empty() || bufferSize64 > UINT_MAX)
	{
		assert(false);
		return;
	}
	const uint32 bufferSize = static_cast<uint32>(bufferSize64);

	ComPtr<ID3D12Resource> indexBuffer;
	std::wstring meshName = GetName().empty() ? L"UnnamedMesh" : GetName();
	const std::wstring resourceName =
		meshName + L" Index " + std::to_wstring(mVecIndexInfo.size());

	// 정적 인덱스 데이터 기본 힙으로 복사
	const bool created = RENDERMANAGER.GetGraphicsCmdQueue()->CreateStaticBuffer(
		buffer.data(),
		bufferSize64,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indexBuffer,
		resourceName.c_str());

	if (!created)
	{
		assert(false);
		return;
	}

	GpuResourceBudget::RecordResource(DEVICE.Get(), L"MeshIndexDefault",
		resourceName + L" " + std::to_wstring(indexCount),
		D3D12_HEAP_TYPE_DEFAULT, indexBuffer.Get());

	D3D12_INDEX_BUFFER_VIEW	indexBufferView;
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = bufferSize;

	IndexBufferInfo info =
	{
		indexBuffer,
		indexBufferView,
		DXGI_FORMAT_R32_UINT,
		indexCount
	};

	mVecIndexInfo.push_back(info);
}

void Mesh::BuildLocalOBBFromVertices(const vector<Vertex>& vertices, BoundingOrientedBox& outOBB)
{
	if (vertices.empty())
	{
		outOBB.Center = XMFLOAT3(0.f, 0.f, 0.f);
		outOBB.Extents = XMFLOAT3(0.5f, 0.5f, 0.5f);
		outOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		return;
	}

	vector<Vec3> positions(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		positions[i] = vertices[i].pos;
	}

	BoundingBox aabb;
	BoundingBox::CreateFromPoints(
		aabb,
		positions.size(),
		positions.data(),
		sizeof(Vec3));

	outOBB.Center = aabb.Center;
	outOBB.Extents = aabb.Extents;
	outOBB.Orientation = XMFLOAT4(0.f, 0.f, 0.f, 1.f);

}

void Mesh::Render(uint32 instanceCount, uint32 idx, uint32 baseInstance, uint32 instancingID )
{
	//Input Assembler (IA)
	//GRAPHICS_CMD_LIST->IASetPrimitiveTopology ( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );	//type : 삼각형 설정
	GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &mVertexBufferView); // Slot: (0~15) _vertexBufferView를 이용해서 데이터 세부사항 설명
	//버텍스 정보 넘김
	GRAPHICS_CMD_LIST->IASetIndexBuffer(& mVecIndexInfo[idx].BufferView);	//인덱스 정보 넘김


	//CMD_LIST->DrawInstanced ( _vertexCount , 1 , 0 , 0 );	// 버텍스 버퍼로 그림을 그려라
	GRAPHICS_CMD_LIST->DrawIndexedInstanced(mVecIndexInfo[idx].Count, instanceCount, baseInstance, 0, instancingID);	//인덱스로 그림을 그려라
}



void Mesh::Render(shared_ptr<class InstancingBuffer>& buffer, uint32 idx)
{
	//buffer을 1개 이상 넣음

	D3D12_VERTEX_BUFFER_VIEW bufferViews[] = { mVertexBufferView, buffer->GetBufferView() };
	GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 2, bufferViews);
	GRAPHICS_CMD_LIST->IASetIndexBuffer(&mVecIndexInfo[idx].BufferView);


	GRAPHICS_CMD_LIST->DrawIndexedInstanced(mVecIndexInfo[idx].Count, buffer->GetCount(), 0, 0, 0);
}



void Mesh::Load(const wstring& path) {

}

void Mesh::CreateMesh(FBXBMeshInfo& f)
{

	BuildLocalOBBFromVertices(f.Vertices, mLocalOBB);
	CreateVertexBuffer(f.Vertices);
	for (const vector<uint32>& buffer : f.Indices)
	{
		if (buffer.empty())
		{
			vector<uint32> defaultBuffer{ 0 };
			CreateIndexBuffer(defaultBuffer);
		}
		else
		{
			CreateIndexBuffer(buffer);
		}
	}

}


CollisionMesh::CollisionMesh() : Object(OBJECT_TYPE::COLLIDER)
{
}

CollisionMesh::~CollisionMesh()
{
}

void CollisionMesh::CreateMesh(FBXBMeshInfo& f)
{
	CreateVertexBuffer(f.Vertices);

	mIndexBuffer.clear();
	for (const vector<uint32>& buffer : f.Indices)
		mIndexBuffer.insert(mIndexBuffer.end(), buffer.begin(), buffer.end());
}

void CollisionMesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	mVertexBuffer = buffer;
}

