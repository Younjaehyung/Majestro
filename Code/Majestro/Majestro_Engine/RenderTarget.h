#pragma once
#include "Texture.h"

enum class RENDER_TARGET_GROUP_TYPE : uint8
{
	SWAP_CHAIN, // BACK_BUFFER, FRONT_BUFFER
	SCENE_SWAP_CHAIN,
	MSAA_SWAP_CHAIN,
	
	SHADOW, // SHADOW
	PRE_DEPTH, // 그림자 맵 렌더링 시 깊이 정보만 필요한 경우 (예: 라이트 구체 계산)
	G_BUFFER, // POSITION, NORMAL, COLOR 
	LIGHTING, // DIFFUSE LIGHT, SPECULAR LIGHT
	HDR, // SCENE HDR COLOR

	POST_HDR_A, //HDR 등 효과 처리용 RT
	POST_HDR_B, //HDR 등 효과 처리용 RT
	POST_LDR_A, //LDR 등 효과 처리용 RT
	POST_LDR_B, //LDR 등 효과 처리용 RT
	MOTION_VECTOR, // 모션 벡터 (velocity.xy)

	// Bloom 멀티스케일 피라미드
	// Bright Extract → 다운샘플 체인 → 업샘플 누적 → Composite
	BLOOM_MIP0,   // 1/2 해상도 (bright extract 대상 & 최종 bloom 누적 결과)
	BLOOM_MIP1,   // 1/4 해상도
	BLOOM_MIP2,   // 1/8 해상도
	BLOOM_MIP3,   // 1/16 해상도 (leaf — 가장 넓은 글로우 기여)
	END,
};

enum class Deferrd_TARGET_GROUP_TYPE : uint8
{
	//SHADOW, // SceneTarget으로 바꿔야 되지 않나?
	POSITION,
	NORMAL,
	COLOR,
	DIFFUSE,
	SPECULAR,
	END,
};

enum class SHADOW_TARGET_TYPE : uint8	// CastShadow 맵 용도
{
	SHADOW_CASCADE0,
	SHADOW_CASCADE1,
	SHADOW_CASCADE2,
	SHADOW_CASCADE3,
	END,
};

enum class G_BUFFER_TYPE : uint8
{
	POSITION, 
	NORMAL, 
	COLOR ,
	END,
};

enum class LIGHTING_TARGET_TYPE : uint8
{
	DIFFUSE,
	SPECULAR,
	END,
};

enum
{
	RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT = static_cast<uint8>(SHADOW_TARGET_TYPE::END),
	RENDER_TARGET_G_BUFFER_GROUP_MEMBER_COUNT = static_cast<uint8>(G_BUFFER_TYPE::END),
	RENDER_TARGET_LIGHTING_GROUP_MEMBER_COUNT = static_cast<uint8>(LIGHTING_TARGET_TYPE::END),
	RENDER_TARGET_GROUP_COUNT = static_cast<uint8>(RENDER_TARGET_GROUP_TYPE::END)
};

struct RenderTarget
{
	shared_ptr<Texture> Target;
	float ClearColor[4] = {0.0f,0.0f ,0.0f ,0.0f };

	D3D12_CPU_DESCRIPTOR_HANDLE		mHeapBegin{};
	int32	TReg = 0;
};

class RenderTargetGroup{
	
public:
	void Create(RENDER_TARGET_GROUP_TYPE groupType, vector<RenderTarget>& rtStru, shared_ptr<Texture> dsTexture);



	void ClearRenderTargetView(uint32 index);
	void ClearRenderTargetView();


	void OMSetRenderTargets(uint32 count, uint32 offset);
	void OMSetRenderTargets();
	// ForwardPass Rim Light용: DEPTH_READ | PIXEL_SHADER_RESOURCE 상태에서 depth test 수행
	void OMSetRenderTargetsReadOnlyDepth();

	void WaitTargetToResource();
	void WaitResourceToTarget();


	vector<RenderTarget>& GetRTG() { return mRenderTargets;}

	shared_ptr<Texture> GetRTTexture(uint8 type) { return mRenderTargets[type].Target; }
	shared_ptr<Texture> GetDSTexture() { return mDepthStencilTexture; }
	RENDER_TARGET_GROUP_TYPE GetGroupType() { return mGroupType; }

private:
	vector<RenderTarget>			mRenderTargets{};
	vector<D3D12_CPU_DESCRIPTOR_HANDLE> mSliceRTVHandles{};
	vector<D3D12_CPU_DESCRIPTOR_HANDLE> mSliceDSVHandles{};
	shared_ptr<Texture>				mDepthStencilTexture;
	
	RENDER_TARGET_GROUP_TYPE		mGroupType{};
	uint32							mRenderTargetCount{};

	D3D12_CPU_DESCRIPTOR_HANDLE		mRTHeapBegin{};
	D3D12_CPU_DESCRIPTOR_HANDLE		mDSHeapBegin{};
	// read-only DSV: DEPTH_READ | PIXEL_SHADER_RESOURCE 동시 사용 시 필요
	D3D12_CPU_DESCRIPTOR_HANDLE		mDSHeapBeginReadOnly{};

	// D3D12_RESOURCE_BARRIER			mTargetToResource[8];	//타켓에서 리소스로 넘어갈때의 베리어
	// D3D12_RESOURCE_BARRIER			mResourceToTarget[8];	//리소스에서 랜더타켓으로 변환시 넘어가는 베리어

	vector<D3D12_RESOURCE_BARRIER>	mTargetToResource;
	vector<D3D12_RESOURCE_BARRIER>	mResourceToTarget;

	// SHADOW 그룹: initial state가 DEPTH_WRITE이므로 최초 프레임에서 PSR→DEPTH_WRITE 배리어 생략
	bool mFirstUse = true;



};

class RenderTargetHeap
{
public:
	void Initialize();

	ComPtr<ID3D12DescriptorHeap> GetRenderTargetHeap() { return mRenderTargetHeap; }
	
	ComPtr<ID3D12DescriptorHeap> GetDepthStencilHeap() { return mDepthStencilHeap; }

	D3D12_CPU_DESCRIPTOR_HANDLE	GetRtvHeapBegin() { return mRtvHeapBegin; }
	D3D12_CPU_DESCRIPTOR_HANDLE	GetDsvHeapBegin() { return mDsvHeapBegin; }

	void SetRtvIndex(uint32 index) {  mRtvLastIndex = index; }
	uint32 GetRtvIndex() { return mRtvLastIndex; }

	void SetDsvIndex(uint32 index) {  mDsvLastIndex = index; }
	uint32 GetDsvIndex() { return mDsvLastIndex; }

private:

	ComPtr<ID3D12DescriptorHeap>	mRenderTargetHeap;
	ComPtr<ID3D12DescriptorHeap>	mDepthStencilHeap;

	


private:
	uint32							mRtvHeapSize;
	uint32							mDsvHeapSize;
	D3D12_CPU_DESCRIPTOR_HANDLE		mRtvHeapBegin;
	D3D12_CPU_DESCRIPTOR_HANDLE		mDsvHeapBegin;

	uint32							mRtvLastIndex;
	uint32							mDsvLastIndex;

};
