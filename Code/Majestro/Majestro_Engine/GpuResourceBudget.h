#pragma once

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#include <DirectXTK12/GraphicsMemory.h>
#include "EngineLog.h"

namespace GpuResourceBudget
{
	struct Entry
	{
		std::wstring group;
		std::wstring name;
		D3D12_HEAP_TYPE heapType{};
		D3D12_RESOURCE_DIMENSION dimension{};
		DXGI_FORMAT format{};
		uint64 bytes{};
		uint64 width{};
		uint32 height{};
		uint16 depthOrArraySize{};
	};

	inline std::vector<Entry>& Entries()
	{
		static std::vector<Entry> entries;
		return entries;
	}

	inline std::mutex& Lock()
	{
		static std::mutex lock;
		return lock;
	}

	inline const char* HeapName(D3D12_HEAP_TYPE heapType)
	{
		switch (heapType)
		{
		case D3D12_HEAP_TYPE_DEFAULT:
			return "DEFAULT";
		case D3D12_HEAP_TYPE_UPLOAD:
			return "UPLOAD";
		case D3D12_HEAP_TYPE_READBACK:
			return "READBACK";
		case D3D12_HEAP_TYPE_CUSTOM:
			return "CUSTOM";
		default:
			return "UNKNOWN";
		}
	}

	inline uint64 AllocationBytes(ID3D12Device* device, const D3D12_RESOURCE_DESC& desc)
	{
		if (device == nullptr)
			return 0;

		D3D12_RESOURCE_ALLOCATION_INFO info =
			device->GetResourceAllocationInfo(0, 1, &desc);
		return info.SizeInBytes;
	}
	
	inline void RecordDesc(ID3D12Device* device, const std::wstring& group,
		const std::wstring& name, D3D12_HEAP_TYPE heapType,
		const D3D12_RESOURCE_DESC& desc)
	{
		// GPU 리소스 생성 지점에서 실제 할당 크기를 기록한다
		// TextureBudget에 잡히지 않는 렌더타겟 버퍼 업로드 힙을 확인하기 위한 진단 코드다
		if (!EngineLog::Enabled(EngineLog::Domain::GpuBudget))
			return;

		Entry entry{};
		entry.group = group;
		entry.name = name;
		entry.heapType = heapType;
		entry.dimension = desc.Dimension;
		entry.format = desc.Format;
		entry.bytes = AllocationBytes(device, desc);
		entry.width = desc.Width;
		entry.height = desc.Height;
		entry.depthOrArraySize = desc.DepthOrArraySize;

		std::lock_guard<std::mutex> guard(Lock());
		Entries().push_back(entry);
	}

	// GPU 리소스 생성 시점에서 실제 할당 크기를 기록한다
	inline void RecordResource(ID3D12Device* device, const std::wstring& group,
		const std::wstring& name, D3D12_HEAP_TYPE heapType,
		ID3D12Resource* resource)
	{
		if (resource == nullptr)
			return;

		RecordDesc(device, group, name, heapType, resource->GetDesc());
	}

	// DumpDxgi는 DXGI 기준 전용/공유 GPU 메모리 사용량을 로그로 출력한다
	inline void DumpDxgi(const char* label, IDXGIAdapter3* adapter,
		DirectX::DX12::GraphicsMemory* graphicsMemory = nullptr)
	{
		const double mb = 1024.0 * 1024.0;

		if (EngineLog::Enabled(EngineLog::Domain::DxgiBudget) && adapter != nullptr)
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO localInfo{};
			DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo{};
			if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &localInfo)) &&
				SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo)))
			{
				EngineLog::WriteTagged(EngineLog::Domain::DxgiBudget, label,
					"dedicated=", static_cast<uint32>(localInfo.CurrentUsage / mb), "MB",
					" dedicatedBudget=", static_cast<uint32>(localInfo.Budget / mb), "MB",
					" shared=", static_cast<uint32>(nonLocalInfo.CurrentUsage / mb), "MB",
					" sharedBudget=", static_cast<uint32>(nonLocalInfo.Budget / mb), "MB");
			}
		}

		if (EngineLog::Enabled(EngineLog::Domain::GraphicsMemory) && graphicsMemory != nullptr)
		{
			const auto stats = graphicsMemory->GetStatistics();
			EngineLog::WriteTagged(EngineLog::Domain::GraphicsMemory, label,
				"committed=", static_cast<uint32>(stats.committedMemory / mb), "MB",
				" total=", static_cast<uint32>(stats.totalMemory / mb), "MB",
				" pages=", stats.totalPages,
				" peakCommitted=", static_cast<uint32>(stats.peakCommitedMemory / mb), "MB",
				" peakTotal=", static_cast<uint32>(stats.peakTotalMemory / mb), "MB");
		}
	}


	// Dump 출력 시점에 GPU 리소스 예산 로그를 출력한다
	inline void Dump(const char* label, IDXGIAdapter3* adapter = nullptr,
		DirectX::DX12::GraphicsMemory* graphicsMemory = nullptr)
	{
		if (!EngineLog::Enabled(EngineLog::Domain::GpuBudget))
		{
			DumpDxgi(label, adapter, graphicsMemory);
			return;
		}

		std::vector<Entry> sorted;
		{
			std::lock_guard<std::mutex> guard(Lock());
			sorted = Entries();
		}

		std::sort(sorted.begin(), sorted.end(),
			[](const Entry& a, const Entry& b) { return a.bytes > b.bytes; });

		uint64 total = 0;
		uint64 defaultTotal = 0;
		uint64 uploadTotal = 0;
		uint64 readbackTotal = 0;
		uint64 customTotal = 0;

		std::vector<std::pair<std::wstring, uint64>> groupTotals;

		for (const Entry& entry : sorted)
		{
			total += entry.bytes;
			switch (entry.heapType)
			{
			case D3D12_HEAP_TYPE_DEFAULT:
				defaultTotal += entry.bytes;
				break;
			case D3D12_HEAP_TYPE_UPLOAD:
				uploadTotal += entry.bytes;
				break;
			case D3D12_HEAP_TYPE_READBACK:
				readbackTotal += entry.bytes;
				break;
			case D3D12_HEAP_TYPE_CUSTOM:
				customTotal += entry.bytes;
				break;
			default:
				break;
			}

			auto found = std::find_if(groupTotals.begin(), groupTotals.end(),
				[&entry](const auto& item) { return item.first == entry.group; });
			if (found == groupTotals.end())
				groupTotals.emplace_back(entry.group, entry.bytes);
			else
				found->second += entry.bytes;
		}

		std::sort(groupTotals.begin(), groupTotals.end(),
			[](const auto& a, const auto& b) { return a.second > b.second; });

		const double mb = 1024.0 * 1024.0;
		EngineLog::WriteTagged(EngineLog::Domain::GpuBudget, label,
			"count=", sorted.size(),
			" total=", static_cast<uint32>(total / mb), "MB",
			" default=", static_cast<uint32>(defaultTotal / mb), "MB",
			" upload=", static_cast<uint32>(uploadTotal / mb), "MB",
			" readback=", static_cast<uint32>(readbackTotal / mb), "MB",
			" custom=", static_cast<uint32>(customTotal / mb), "MB");

		DumpDxgi(label, adapter, graphicsMemory);

		const size_t groupN = (std::min)(static_cast<size_t>(12), groupTotals.size());
		for (size_t i = 0; i < groupN; ++i)
		{
			EngineLog::WriteTagged(EngineLog::Domain::GpuBudget, label,
				"group=", EngineLog::Narrow(groupTotals[i].first),
				" size=", static_cast<uint32>(groupTotals[i].second / mb), "MB");
		}

		const size_t topN = (std::min)(static_cast<size_t>(30), sorted.size());
		for (size_t i = 0; i < topN; ++i)
		{
			const Entry& entry = sorted[i];
			EngineLog::WriteTagged(EngineLog::Domain::GpuBudget, label,
				"resource=", EngineLog::Narrow(entry.name),
				" group=", EngineLog::Narrow(entry.group),
				" heap=", HeapName(entry.heapType),
				" size=", static_cast<uint32>(entry.bytes / mb), "MB",
				" desc=", entry.width, "x", entry.height, "x",
				entry.depthOrArraySize);
		}
	}
}
