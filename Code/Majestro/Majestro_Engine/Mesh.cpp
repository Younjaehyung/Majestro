#include "pch.h"
#include "Mesh.h"
#include "Engine.h"
#include "RenderManager.h"
#include "Material.h"
#include "FBXData.h"



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
	uint32 bufferSize = mVertexCount * sizeof(Vertex);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);	//버퍼타입 : UPLOAD
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	//VRAM(Upload)에 버텍스버퍼 생성
	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mVertexBuffer));

	// Copy the triangle data to the vertex buffer.
	void* vertexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	mVertexBuffer->Map(0, &readRange, &vertexDataBuffer);	//메모리 연결
	::memcpy(vertexDataBuffer, &buffer[0], bufferSize);	//gpu로 메모리 복사
	//_vertexBuffer->Unmap(0, nullptr);	//메모리 연결 해제

	// 리소스(데이터) 정보(속성) 설정
	// Initialize the vertex buffer view.
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();	// GPU VRAM의 주소를 지정
	mVertexBufferView.StrideInBytes = sizeof(Vertex); // 정점 1개 크기
	mVertexBufferView.SizeInBytes = bufferSize; // 버퍼의 크기	
}

void Mesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	uint32 indexCount = static_cast<uint32>(buffer.size());
	uint32 bufferSize = indexCount * sizeof(uint32);

	D3D12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	ComPtr<ID3D12Resource> indexBuffer;
	DEVICE->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer));

	void* indexDataBuffer = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	indexBuffer->Map(0, &readRange, &indexDataBuffer);
	::memcpy(indexDataBuffer, &buffer[0], bufferSize);
	indexBuffer->Unmap(0, nullptr);

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

	BoundingOrientedBox::CreateFromPoints(
		outOBB,
		positions.size(),
		positions.data(),
		sizeof(Vec3));
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

void CollisionMesh::CreateVertexBuffer(const vector<Vertex>& buffer)
{
	mVertexBuffer = buffer;
}

void CollisionMesh::CreateIndexBuffer(const vector<uint32>& buffer)
{
	mIndexBuffer = buffer;
}

void CollisionMesh::CreateCollisionMesh(FBXBMeshInfo& f)
{
	CreateVertexBuffer(f.Vertices);

	if (mVertexBuffer.empty())
		return;

	// 모든 정점의 위치를 추출
	vector<Vec3> positions(mVertexBuffer.size());
	for (size_t i = 0; i < mVertexBuffer.size(); ++i)
	{
		positions[i] = mVertexBuffer[i].pos;
	}

	// 모든 점을 한 번에 전달하여 OBB 생성
	BoundingOrientedBox::CreateFromPoints(
		mOBB,
		positions.size(),
		positions.data(),
		sizeof(Vec3)
	);
}
