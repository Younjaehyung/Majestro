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

enum class CBV_REGISTER :uint8 {
	b0,	// rootConstant용 (INDEX ID)
	b1, 
	b2,
	b3,
	b4,

	END
};

enum class SRV_REGISTER :uint8 {
	t0 = static_cast<uint8>(CBV_REGISTER::END),
	t1,
	t2,
	t3,
	t4,
	t5,
	t6,
	t7,
	t8,
	t9,

	END
};



//compute 쉐이더용
enum class UAV_REGISTER : uint8
{
	u0 = static_cast<uint8>(SRV_REGISTER::END),
	u1,
	u2,
	u3,
	u4,

	END,
};

enum class CONSTANT_INDEX : uint8
{ // b레지스터
	CBV_CAMERA_INDEX = static_cast<uint8>(CBV_REGISTER::b1),
	CBV_ETC_INDEX,

	CBV_INDEX_END
};

enum class STRUCTURED_INDEX : uint8
{ // t레지스터
	SRV_LIGHT_INDEX,
	SRV_TRANSFROM_INDEX,
	SRV_MATERIALS_INDEX,
	SRV_BONE_INDEX,
	SRV_PARTICLE_INDEX,
	UAV_PARTICLE_INDEX,
	/*SRV_BONE_INDEX, */

	SRV_INDEX_END
};

enum class TEXTURE_INDEX : uint8
{ // t레지스터
	TEXTURE_INDEX = static_cast<uint8>(STRUCTURED_INDEX::SRV_INDEX_END),


	TEXTURE_INDEX_END
};


enum {
	SWAP_CHAIN_BUFFER_COUNT = 2	//더블버퍼링 버퍼 개수
	, CBV_REGISTER_COUNT = CBV_REGISTER::END
	, SRV_REGISTER_COUNT = static_cast<uint8>(SRV_REGISTER::END) - CBV_REGISTER_COUNT
	, CBV_SRV_REGISTER_COUNT = CBV_REGISTER_COUNT + SRV_REGISTER_COUNT	//CBV,SRV의 레지스터 개수
	,UAV_REGISTER_COUNT = static_cast<uint8>(UAV_REGISTER::END) - CBV_SRV_REGISTER_COUNT
	, TEXTURE_DESCRIPTOR_COUNT =	4000	// texture(descriptors) 개수
	,TOTAL_REGISTER_COUNT = CBV_SRV_REGISTER_COUNT + UAV_REGISTER_COUNT	//총 레지스터 개수
	, GROUP_COUNT = 3
};


enum class mRootParmetersIndex : uint8
{
	CONSTANT,
	TABLE,
	SAMPLER,
	/*TABLE(UAV),*/

	END
};



struct WindowInfo {
	HWND	Hwnd;		//출력 윈도우 핸들
	int32	Width;		//너비
	int32	Height;	//높이
	bool	ScreenState;	//전체,창 모드
};



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

#define CONST_BUFFER(type) gEngine->GetRenderManager().GetConstantBuffer(type)
