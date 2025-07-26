#pragma once
#include "Buffer.h"



class InstancingManager
{

public:
	void Render(unordered_map<std::wstring&, std::vector<Entity>>& gameObjects);

	void ClearBuffer();
	void Clear() { _buffers.clear(); }

private:
	void AddParam(uint64 instanceId, InstancingParams& data);

private:
	map<uint64/*instanceId*/, shared_ptr<InstancingBuffer>> _buffers;
};


