#pragma once

// ���� include
#include <windows.h>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
using namespace std;

#include "d3dx12.h"
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



// ���� typedef
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

using Vec		= XMVECTOR;	
using Vec2		= XMFLOAT2;
using Vec3		= XMFLOAT3;
using Vec4		= XMFLOAT4;
using Matrix	= XMMATRIX;


enum class CBV_REGISTER :uint8 {
	b0,
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


struct WindowInfo {
	HWND	Hwnd;		//출력 윈도우 핸들
	int32	Width;		//너비
	int32	Height;	//높이
	bool	ScreenState;	//전체,창 모드
};

extern unique_ptr<class Engine> gEngine;


#define DEVICE	 gEngine->GetRenderManager().GetDevice()->GetDevice()
#define GRAPHICS_CMD_LIST gEngine->GetRenderManager()->GetGraphicsCmdQueue ( )->GetGraphicsCmdList ( )
#define COMPUTE_CMD_LIST gEngine->GetRenderManager()->GetComputeCmdQueue ( )->GetComputeCmdList ( )

#define GRAPHICS_ROOT_SIGNATURE gEngine->GetRenderManager()->GetRootSignature()->GetGraphicsRootSignature()
#define COMPUTE_ROOT_SIGNATURE gEngine->GetRenderManager()->GetRootSignature()->GetComputeRootSignature()


#define RESOURCE_CMD_LIST gEngine->GetGraphicsCmdQueue()->GetResourceCmdList()

#define DELTA_TIME		GET_SINGLE(Timer)->GetDeltaTime()
#define INPUT	GET_SINGLE(Input)

#define CONST_BUFFER(type) gEngine->GetConstantBuffer(type)
