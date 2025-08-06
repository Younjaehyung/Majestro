#pragma once

//std::byte 사용안함 설정
#define _HAS_STD_BYTE 0

// ���� include
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <filesystem>
using namespace std;





#include "d3dx12.h"
#include "SimpleMath.h"
#include <d3d12.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;


// ���� lib
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/DirectXTex.inl>

// DEBUG

#pragma comment(linker,"/entry:wWinMainCRTStartup /subsystem:console")

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex\\DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex\\DirectXTex.lib")
#endif



using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

using Vec		= XMVECTOR;	
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;


struct Vertex {
	Vertex() {}

	Vertex(Vec3 p, Vec2 u, Vec3 n, Vec3 t)
		: pos(p), uv(u), normal(n), tangent(t)
	{
	}

	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;
};

struct WindowInfo {
	HWND	Hwnd;		//출력 윈도우 핸들
	int32	Width;		//너비
	int32	Height;	//높이
	bool	ScreenState;	//전체,창 모드
};


////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 RootSignature기준 세팅시 ROOT_PARAMETER로 부여된						  //
//		reg의 번호를 정의 해둠.															  //
//		RootConstant와 DescriptorTable만을 사용함.
////////////////////////////////////////////////////////////////////////////////////////////



enum class mRootParmetersIndex : uint8
{
	CONSTANT,		//	[0]
	TABLE_GROUP,	//	[1]
	TABLE_TEXTURE,	//	[2]
	SAMPLER,		//	[3]


	END
};


enum class RCONSTANT_INDEX : uint8		//rootConstant
{ // b레지스터
	RCONSTANT_INDEX_PARM,	// 랜더링 파라미터 b0

	RCONSTANT_INDEX_END
};


enum class CONSTANT_INDEX : uint8		//DescriptorTable CBV
{ // b레지스터 (암묵적 space0)
	CBV_CAMERA_INDEX = static_cast<uint8>(RCONSTANT_INDEX::RCONSTANT_INDEX_END),	// 카메라 파라미터 b1

	CBV_INDEX_END
};


enum class GBUFFER_INDEX : uint8		//DescriptorTable SRV
{ // t레지스터 space 0 
	GBUFFER_SHADOW_INDEX,
	
	GBUFFER_POSITION_INDEX,
	GBUFFER_NORMAL_INDEX,
	GBUFFER_ALBEDO_INDEX,

	GBUFFER_DIFFUSE_INDEX,
	GBUFFER_SPECULAR_INDEX,
	


	GBUFFER_INDEX_END
};

enum class STRUCTURED_INDEX : uint8		//DescriptorTable SRV&UAV
{ // t레지스터 space 1
	SRV_LIGHT_INDEX,
	SRV_OBJECTINFO_INDEX,
	SRV_MATERIALS_INDEX,
	SRV_BONE_INDEX,
	SRV_PARTICLE_INDEX,
	UAV_PARTICLE_INDEX,
	/*SRV_BONE_INDEX, */

	SRV_INDEX_END
};

enum class UAV_INDEX : uint8
{
	UAV_PARTICLE_INDEX,
	UAV_TEXTURE_INDEX,
	UAV_INDEX_END
};

enum class TEXTURE_INDEX : uint8		//DescriptorTable SRV(TEXTURE)
{ // t레지스터 space 2
	TEXTURE_INDEX,

	TEXTURE_INDEX_END
};


enum {	// space 번호
	GBUFFER_SPACE = 0
	, STRUCTURED_SPACE = 1
	, TEXTURE_SPACE = 2

};

////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 DescriptorHeap기준 세팅시 Group별로 부여된							  //
//		위치 인덱스를 정의 해둠.															  
////////////////////////////////////////////////////////////////////////////////////////////

enum {


	, SWAP_CHAIN_BUFFER_COUNT = 2	// 더블버퍼링 버퍼 개수
	, FRAMEGROUP_COUNT = 2			// 추후 프레임리소스 선택시 3으로 변경할것.


	, TEXTURE_SRV_COUNT = 1000	// texture(SRV) 개수
	, TEXTURE_UAV_COUNT = 1000	// texture(UAV) 개수


	, GBUFFER_INDEX_START = 0
	, GBUFFER_INDEX_COUNT = static_cast<uint8>(GBUFFER_INDEX::GBUFFER_INDEX_END)
	
	, CONSTANT_INDEX_START = (GBUFFER_INDEX_COUNT)*FRAMEGROUP_COUNT
	, CONSTANT_INDEX_COUNT = static_cast<uint8>(CONSTANT_INDEX::CBV_INDEX_END) - 1


	, STRUCTURED_INDEX_START = (CONSTANT_INDEX_COUNT+ GBUFFER_INDEX_COUNT) * FRAMEGROUP_COUNT
	, STRUCTURED_INDEX_COUNT = static_cast<uint8>(STRUCTURED_INDEX::SRV_INDEX_END)
	

	, GROUP_COUNT = GBUFFER_INDEX_COUNT + CONSTANT_INDEX_COUNT + STRUCTURED_INDEX_COUNT

	, UAV_INDEX_START = GROUP_COUNT * FRAMEGROUP_COUNT
	, UAV_INDEX_COUNT = TEXTURE_UAV_COUNT + 1		//UAV_TEXTURE + UAV_STRUCTURED(1)

	, TEXTURE_INDEX_START = UAV_INDEX_START + UAV_INDEX_COUNT
	, TEXTURE_INDEX_COUNT = TEXTURE_SRV_COUNT
	, TEXTURE_COUNT = TEXTURE_SRV_COUNT
	
	, 

	, ALL_DESCRIPTOR_COUNT = FRAMEGROUP_COUNT* (CONSTANT_INDEX_COUNT + GBUFFER_INDEX_COUNT + STRUCTURED_INDEX_COUNT) + TEXTURE_INDEX_COUNT
};




////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 개발시 빠르게 전역매니저로 접근 가능한								  //
//		Helper 매크로 함수들을 정의 해둠.
////////////////////////////////////////////////////////////////////////////////////////////



extern unique_ptr<class Engine> gEngine;

#define RENDERMANAGER	gEngine->GetRenderManager()
#define RESOURCEMANAGER	gEngine->GetResourceManager()

#define DEVICE	 gEngine->GetRenderManager().GetDevice()->GetDevice()
#define GRAPHICS_CMD_LIST gEngine->GetRenderManager().GetGraphicsCmdQueue ( )->GetGraphicsCmdList ( )
#define Graphics_DescHeap gEngine->GetRenderManager().GetGraphicsDescHeap()
//#define COMPUTE_CMD_LIST gEngine->GetRenderManager().GetComputeCmdQueue ( )->GetComputeCmdList ( )

//#define GRAPHICS_ROOT_SIGNATURE gEngine->GetRenderManager().GetRootSignature()->GetGraphicsRootSignature()
//define COMPUTE_ROOT_SIGNATURE gEngine->GetRenderManager()->GetRootSignature()->GetComputeRootSignature()


#define RESOURCE_CMD_LIST gEngine->GetRenderManager().GetGraphicsCmdQueue()->GetResourceCmdList()

#define DELTA_TIME		gEngine->GetTimer().GetTimeElapsed()

#define CONST_BUFFER(type,count) gEngine->GetRenderManager().GetConstantBuffer(type,count)
#define STRUCT_BUFFER(type,count) gEngine->GetRenderManager().GetStructuredBuffer(type,count)
