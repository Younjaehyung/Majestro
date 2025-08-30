#include "pch.h"
#include "Mesh.h"
#include "Engine.h"
#include "RenderManager.h"
#include "Material.h"

Mesh::Mesh() : Object(OBJECT_TYPE::MESH)
{
}

Mesh::~Mesh()
{
}

void Mesh::Init(const vector<Vertex>& vec, const vector<uint32>& indexbuffer)
{
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

void Mesh::Render(uint32 instanceCount, uint32 idx, uint32 baseInstance )
{
	//Input Assembler (IA)
	//GRAPHICS_CMD_LIST->IASetPrimitiveTopology ( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );	//type : 삼각형 설정
	GRAPHICS_CMD_LIST->IASetVertexBuffers(0, 1, &mVertexBufferView); // Slot: (0~15) _vertexBufferView를 이용해서 데이터 세부사항 설명
	//버텍스 정보 넘김
	GRAPHICS_CMD_LIST->IASetIndexBuffer(& mVecIndexInfo[idx].BufferView);	//인덱스 정보 넘김


	//CMD_LIST->DrawInstanced ( _vertexCount , 1 , 0 , 0 );	// 버텍스 버퍼로 그림을 그려라
	GRAPHICS_CMD_LIST->DrawIndexedInstanced(mVecIndexInfo[idx].Count, instanceCount, baseInstance, 0, 0);	//인덱스로 그림을 그려라
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