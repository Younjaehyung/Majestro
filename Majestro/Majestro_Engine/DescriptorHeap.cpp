#include "pch.h"
#include "DescriptorHeap.h"
#include "Engine.h"
#include "RenderManager.h"


void GraphicsDescriptorHeap::Initialize(uint32 count)
{
	mGroupCount = count;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};	//DESCRIPTOR HEAP 세팅
	desc.NumDescriptors = count * (CBV_SRV_REGISTER_COUNT - 1) ;	//b0로 전역이기에 1개를 뺌
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mDescHeap));	//DESCRIPTOR 힙(테이블) 생성

	mHandleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mGroupSize = mHandleSize * (CBV_SRV_REGISTER_COUNT - 1);	//b0로 전역이기에 1개를 뺌
}

void GraphicsDescriptorHeap::Initialize(uint32 count)	// 추후 수정중 (count = 프레임리소스의 개수)
{
	mGroupCount = count;

	D3D12_DESCRIPTOR_HEAP_DESC desc{};	//DESCRIPTOR HEAP 세팅
	desc.NumDescriptors = count * (CBV_SRV_REGISTER_COUNT - 1) + TEXTURE_DESCRIPTOR_COUNT;	//b0로 전역이기에 1개를 뺌
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;	

	DEVICE->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mDescHeap));	//DESCRIPTOR 힙(테이블) 생성

	mHandleSize = DEVICE->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mGroupSize = mHandleSize * (CBV_SRV_REGISTER_COUNT - 1);	//b0로 전역이기에 1개를 뺌
	mTextureGroupIndex = mGroupCount * (CBV_SRV_REGISTER_COUNT - 1);	// mTextureGroupIndex의 시작 위치
}

void GraphicsDescriptorHeap::Clear()
{
	mCurrentGroupIndex = 0;
}

void GraphicsDescriptorHeap::SetCBV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, CBV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//CPU Desc Heap에 있던 view값들을 GPU Desc Heap(table)로 복사 함
}

void GraphicsDescriptorHeap::SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE srcHandle, SRV_REGISTER reg)
{
	D3D12_CPU_DESCRIPTOR_HANDLE destHandle = GetCPUHandle(reg);

	uint32 destRange = 1;
	uint32 srcRange = 1;
	DEVICE->CopyDescriptors(1, &destHandle, &destRange, 1, &srcHandle, &srcRange, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//CPU Desc Heap에 있던 view값들을 GPU Desc Heap(table)로 복사 함
}



void GraphicsDescriptorHeap::CommitTable()
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += mCurrentGroupIndex * mGroupSize;
	GRAPHICS_CMD_LIST->SetGraphicsRootDescriptorTable(1, handle);
	//CMD를 통하여 Desc Table에 있는 값들을 레지스터에 보내는 명령어를 실행.(CMD이기 때문에 즉시가 아니라 나중에 실행됨)

	mCurrentGroupIndex++;
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(CBV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(SRV_REGISTER reg)
{
	return GetCPUHandle(static_cast<uint8>(reg));
}

D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDescriptorHeap::GetCPUHandle(uint8 reg)
{
	assert(reg > 0);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += mCurrentGroupIndex * mGroupSize;	//그룹 사이즈 이동
	handle.ptr += (reg - 1) * mHandleSize;	//레지스터(핸들) 이동 , b0은 전역으로 활용하고 b1부터 사용하기에 1개를 뺌
	return handle;
}

