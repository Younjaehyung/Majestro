#pragma once
#include "Object.h"

class Material;

// [유니티짱]과 같이 정점으로 이루어진 물체
class Mesh : public Object
{
public:

	Mesh();
	virtual ~Mesh();

	void Init(const vector<Vertex>& vec, const vector<uint32>& indexbuffer);
	void Render(uint32 instanceCount = 1);
	void Render(shared_ptr<class InstancingBuffer>& buffer);
private:

	void CreateVertexBuffer(const vector<Vertex>& buffer);
	void CreateIndexBuffer(const vector<uint32>& buffer);
private:
	ComPtr<ID3D12Resource>		_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW	_vertexBufferView = {};
	uint32 _vertexCount = 0;

	ComPtr<ID3D12Resource>		_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW	_indexBufferView;
	uint32 _indexCount = 0;

	//bufferView : buffer의 주소값과 정보들이 담겨있음

};

