#include "pch.h"
#include "DescriptorHeap.h"
#include "Engine.h"
#include "RenderManager.h"


void GraphicsDescriptorHeap::Initialize(uint32 count)	// 추후 수정중 (count = 프레임리소스의 개수)
{
	mGroupCount = count;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};	//DESCRIPTOR HEAP 세팅
	desc.NumDescriptors = ALL_DESCRIPTOR_COUNT+100;	//b0로 전역이기에 1개를 뺌
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mDescHeap));	//DESCRIPTOR 힙(테이블) 생성

	mHandleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mGroupSize = mHandleSize * (desc.NumDescriptors);	//b0로 전역이기에 1개를 뺌
	mTextureGroupIndex = static_cast<uint32>(TEXTURE_INDEX::TEXTURE_INDEX);	// mTextureGroupIndex의 시작 위치
	mLastIndex = mTextureGroupIndex;
}


void GraphicsDescriptorHeap::CommitTable(uint32 frameCount,uint32 signautreNum,uint32 tableBegin, uint32 tableGroupSize)
{


	D3D12_GPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetGPUDescriptorHandleForHeapStart();

	// 만약 tableGroupSize가 0 이면 이것은 프레임리소스를 사용안한다고 간주하게됨. 
	handle.ptr += static_cast<uint64>(( tableBegin +( frameCount * tableGroupSize))* mHandleSize);
	GRAPHICS_CMD_LIST->SetGraphicsRootDescriptorTable(signautreNum, handle);
	//CMD를 통하여 Desc Table에 있는 값들을 레지스터에 보내는 명령어를 실행.(CMD이기 때문에 즉시가 아니라 나중에 실행됨)



}
