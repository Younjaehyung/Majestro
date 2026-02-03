#pragma once

//std::byte 사용안함 설정
#define _HAS_STD_BYTE 0

// Network
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32.lib")

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
#include <initializer_list>
#include <filesystem>
#include <fstream>
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


// DirectXTK12
#include <DirectXTK12/ResourceUploadBatch.h >
#include <DirectXTK12/SpriteBatch.h >

//#include <DirectXTK12/CommonStates.h >
//#include <DirectXTK12/DescriptorHeap.h >
#include <DirectXTK12/SpriteFont.h >
//#include <DirectXTK12/GraphicsMemory.h >
#ifdef _DEBUG
#pragma comment(lib, "DirectXTK12/DirectXTK12_D.lib")
#else
#pragma comment(lib, "DirectXTK12/DirectXTK12.lib")
#endif



//#define _IMGUI


// DEBUG
#pragma comment(linker,"/entry:wWinMainCRTStartup /subsystem:console")

// IMGUI

#include <ImGUI/imgui.h>
#include <ImGUI/imgui_impl_win32.h>
#include <ImGUI/imgui_impl_dx12.h>
#ifdef _IMGUI
#pragma comment(lib, "ImGUI\\example_win32_directx12.lib")
#else
#endif

// DirectXTex
#include <DirectXTex/DirectXTex.h>
#include <DirectXTex/DirectXTex.inl>

#ifdef _DEBUG
#pragma comment(lib, "DirectXTex\\DirectXTex_Debug.lib")
#else
#pragma comment(lib, "DirectXTex\\DirectXTex.lib")
#endif

// FMOD
#include <FMod/fmod.hpp>
#include <FMod/fmod_studio.hpp>
#include <FMod/fmod_errors.h>

#ifdef _DEBUG
#pragma comment(lib, "FMod/fmodL_vc.lib")
#pragma comment(lib, "FMod/fmodstudioL_vc.lib")
#else
#pragma comment(lib, "FMod/fmod_vc.lib")
#pragma comment(lib, "FMod/fmodstudio_vc.lib")
#endif

// EFFEKSEER
#include <Effekseer/Effekseer/Effekseer.h>
#include <Effekseer/EffekseerRendererDX12/EffekseerRendererDX12.h>
#include <Effekseer/LLGI/LLGI.Base.h>

#ifdef _DEBUG
#pragma comment(lib, "Effekseer/Effekseer_D.lib")
#pragma comment(lib, "Effekseer/EffekseerRendererDX12_D.lib")
#pragma comment(lib, "Effekseer/LLGI_D.lib")
#else
#pragma comment(lib, "Effekseer/Effekseer.lib")
#pragma comment(lib, "Effekseer/EffekseerRendererDX12.lib")
#pragma comment(lib, "Effekseer/LLGI.lib")
#endif




///////////////////////////////////////////////////////////////////////////////
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
	Vertex(Vec3 p, Vec2 u)
		: pos(p), uv(u)
	{
	}
	Vertex(Vec3 p, Vec2 u, Vec3 n, Vec3 t)
		: pos(p), uv(u), normal(n), tangent(t)
	{
	}

	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;


	Vec4 weights;
	Vec4 indices;
};
///////////////////////////////////////////////////////////////////////////////




struct WindowInfo {
	HWND	Hwnd;		//출력 윈도우 핸들
	int32	Width;		//너비
	int32	Height;	//높이
	bool	ScreenState;	//전체,창 모드
};

wstring s2ws(const string& s);
string ws2s(const wstring& s);
void LogDebug(const std::string& msg);
void LogDebugW(const std::wstring& msg);

// network
void err_quit(const char* msg);
void err_display(const char* msg);
void err_display(int errcode);

#include "../../Protocol/Packet.h"
#include "PacketHelper.h"
////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 RootSignature기준 세팅시 ROOT_PARAMETER로 부여된						  //
//		reg의 번호를 정의 해둠.															  //
//		RootConstant와 DescriptorTable만을 사용함.
////////////////////////////////////////////////////////////////////////////////////////////



enum class mRootParmetersIndex : uint8
{
	CONSTANT,		//	[0]
	TABLE_GROUP,	//	[1]
	SRV_PARTICLE,
	TABLE_PARTICLE,	//	[1]
	TABLE_TEXTURE,	//	[2]
	SAMPLER,		//	[3]


	END
};


enum class RCONSTANT_INDEX : uint8		//rootConstant
{ // b레지스터
	RCONSTANT_INDEX_PARM,	// 랜더링 파라미터 b0

	RCONSTANT_INDEX_END
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


enum class CONSTANT_INDEX : uint8		//DescriptorTable CBV
{ // b레지스터 (space1)
	CBV_PASSINFO_INDEX ,	// PASS 파라미터 b0


	CBV_INDEX_END
};


enum class GROUP_SRV_INDEX : uint8		//DescriptorTable SRV
{ // t레지스터 space 1
	SRV_INSTANCE_INDEX,
	SRV_LIGHT_INDEX,
	SRV_OBJECTINFO_INDEX,
	SRV_PARTICLE_INDEX,	// 파티클 system view
	SRV_UI_INDEX,
	SRV_ANIMINSTANCE_INDEX,
	SRV_FINALUBONE_INDEX,
	GROUP_SRV_END,
	//SRV_PARTICLE_INDEX,
	//UAV_PARTICLE_INDEX,


};

enum class GROUP_UAV_INDEX : uint8		//DescriptorTable UAV
{ // t레지스터 space 1
	UAV_FINALUBONE_INDEX,

	GROUP_UAV_END
};



enum class PARTICLE_INDEX : uint8		//
{
	// t레지스터 space 2
	SRV_PARTICLE_INDEX = 0,			// 파티클 읽기 view
	UAV_PARTICLE_INDEX = 0,			// 파티클 쓰기 view
	UAV_PARTICLE_SHARED_INDEX = 1,	// 파티클 shared view 
	PARTICLE_INDEX_END = 3
};

enum class ANIMATION_INDEX : uint8		//
{
	// t레지스터 space 2
	SRV_SKELETONBONE_INDEX,			// 스켈레톤
	SRV_ANIMATIONCLIP_INDEX,		// 애니메이션
	SRV_ANIMATIONMETA_INDEX,		// 에니메이션 메타 
	ANIMATION_INDEX_END
};

enum class TEXTURE_INDEX : uint8		//DescriptorTable SRV(TEXTURE)
{ // t레지스터 space 3
	TEXTURE_MATERIALS_INDEX,
	TEXTURE_CUBE_INDEX,
	TEXTURE_CUBE_INDEX_COUNT = 15,  

	TEXTURE_INDEX ,

	TEXTURE_INDEX_END
};


enum {	// space 번호
	GBUFFER_SPACE = 0
	, STRUCTURED_SPACE = 1
	, PARTICLE_SPACE = 2
	, ANIMATION_SPACE = 3
	, TEXTURE_SPACE = 4

};

////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 DescriptorHeap기준 세팅시 Group별로 부여된							  //
//		위치 인덱스를 정의 해둠.															  
////////////////////////////////////////////////////////////////////////////////////////////

#pragma region DescriptorHeap

////////////////////////////
///  G-BUFFER
///  0
///  1
///  2
///  3
///	 4
///  5
////////////////////////////
///  CBV
///  0
///  SRV
///	 0
///  1
///  2
///  3
///  4
////////////////////////////
///  CBV
///  0
///  SRV
///	 0
///  1
///  2
///  3
///  4
///////////////////////////////
///  CBV
///  0
///  SRV
///	 0
///  1
///  2
///  3
///  4

/// ///////////////////////////////
///  Particle
///  0 - 	SRV_PARTICLE_INDEX,			// 파티클1 읽기 view
///  1 - 	UAV_PARTICLE_INDEX,			// 파티클1 쓰기 view
///  2 - 	UAV_PARTICLE_SHARED_INDEX,	// 파티클1 shared view 
///  3 -	SRV_PARTICLE_INDEX,			// 파티클2 읽기 view
///  4 -	UAV_PARTICLE_INDEX,			// 파티클2 쓰기 view
///  5 -	UAV_PARTICLE_SHARED_INDEX,	// 파티클2 shared view
///////////////////////////////
/// TEXTURE_CUBE
/// 0 ..16(~15)
/// TEXTURE
/// 0 ..1024(~1023)

//////////////////////////////////
#pragma endregion

enum {


	SWAP_CHAIN_BUFFER_COUNT = 2	// 더블버퍼링 버퍼 개수
	, FRAMEGROUP_COUNT = 3			// 추후 프레임리소스 선택시 3으로 변경할것.


	, TEXTURE_SRV_COUNT = 2048	// texture(SRV) 개수
	, TEXTURE_UAV_COUNT = 1024	// texture(UAV) 개수
	, TEXTURE_CUBE_COUNT = 16	// texture(CUBE) 개수
	, PARTICLE_COUNT = 4096		// particle(CUBE) 개수

	, GBUFFER_INDEX_START = 0
	, GBUFFER_INDEX_COUNT = static_cast<uint8>(GBUFFER_INDEX::GBUFFER_INDEX_END)



	, CONSTANT_INDEX_START = (GBUFFER_INDEX_COUNT)
	, CONSTANT_INDEX_COUNT = static_cast<uint8>(CONSTANT_INDEX::CBV_INDEX_END)


	, GROUP_SRV_START = CONSTANT_INDEX_START + CONSTANT_INDEX_COUNT
	, GROUP_SRV_COUNT = static_cast<uint8>(GROUP_SRV_INDEX::GROUP_SRV_END)

	, GROUP_UAV_START = GROUP_SRV_START + GROUP_SRV_COUNT
	, GROUP_UAV_COUNT = static_cast<uint8>(GROUP_UAV_INDEX::GROUP_UAV_END)

	, GROUP_START = GBUFFER_INDEX_START + GBUFFER_INDEX_COUNT
	, GROUP_COUNT = CONSTANT_INDEX_COUNT + GROUP_SRV_COUNT + GROUP_UAV_COUNT

	//, PARTICLE_SYSTEM_START = GROUP_START + (GROUP_COUNT * FRAMEGROUP_COUNT)
	//, PARTICLE_SYSTEM_COUNT = static_cast<uint8>(PARTICLE_SYSTEM::SRV_PARTICLE_SYSTEM_END)

	, PARTICLE_INDEX_START = GROUP_START + (GROUP_COUNT * FRAMEGROUP_COUNT)
	, PARTICLE_GROUP_COUNT = 1// 파티클 시스템 종류 개수 (파티클 이미터 개수)
	, PARTICLE_INDEX_COUNT = (static_cast<uint8>(PARTICLE_INDEX::PARTICLE_INDEX_END)* PARTICLE_GROUP_COUNT)	//UAV_TEXTURE + UAV_STRUCTURED(1)

	, ANIMATION_INDEX_START = PARTICLE_INDEX_START + PARTICLE_INDEX_COUNT
	, ANIMATION_INDEX_COUNT = (static_cast<uint8>(ANIMATION_INDEX::ANIMATION_INDEX_END))	//UAV_TEXTURE + UAV_STRUCTURED(1)

	, TEXTURE_MATERIALS_INDEX_START = ANIMATION_INDEX_START + ANIMATION_INDEX_COUNT
	, TEXTURE_MATERIALS_INDEX_COUNT = 1

	, TEXTURE_CUBE_INDEX_START = TEXTURE_MATERIALS_INDEX_START + TEXTURE_MATERIALS_INDEX_COUNT
	, TEXTURE_CUBE_INDEX_COUNT = TEXTURE_CUBE_COUNT

	, TEXTURE_INDEX_START = TEXTURE_CUBE_INDEX_START + TEXTURE_CUBE_INDEX_COUNT
	, TEXTURE_INDEX_COUNT = TEXTURE_SRV_COUNT
	
	, IMGUI_INDEX_START = TEXTURE_INDEX_START + TEXTURE_INDEX_COUNT
	, IMGUI_INDEX_COUNT = 1

	, UI_INDEX_START = IMGUI_INDEX_START + IMGUI_INDEX_COUNT
	, UI_INDEX_COUNT = 1
	


	, ALL_DESCRIPTOR_COUNT = GBUFFER_INDEX_COUNT + (GROUP_COUNT * FRAMEGROUP_COUNT) + PARTICLE_INDEX_COUNT + 
	TEXTURE_CUBE_INDEX_COUNT + TEXTURE_INDEX_COUNT + IMGUI_INDEX_COUNT + UI_INDEX_COUNT
};




////////////////////////////////////////////////////////////////////////////////////////////
//		이 곳에는 개발시 빠르게 전역매니저로 접근 가능한								  //
//		Helper 매크로 함수들을 정의 해둠.
////////////////////////////////////////////////////////////////////////////////////////////



extern unique_ptr<class Engine> gEngine;

#define RENDERMANAGER	gEngine->GetRenderManager()
#define RESOURCEMANAGER	gEngine->GetResourceManager()
#define AUDIOMANAGER	gEngine->GetAudioManager()

#define DEVICE	 gEngine->GetRenderManager().GetDevice()->GetDevice()
#define GRAPHICS_CMD_LIST gEngine->GetRenderManager().GetGraphicsCmdQueue ()->GetGraphicsCmdList ()
#define RESOURCE_CMD_LIST gEngine->GetRenderManager().GetGraphicsCmdQueue()->GetResourceCmdList()
#define COMPUTE_CMD_LIST  gEngine->GetRenderManager().GetComputeCmdQueue()->GetComputeCmdList()
#define Graphics_DescHeap gEngine->GetRenderManager().GetGraphicsDescHeap()

//#define GRAPHICS_ROOT_SIGNATURE gEngine->GetRenderManager().GetRootSignature()->GetGraphicsRootSignature()
//#define COMPUTE_ROOT_SIGNATURE gEngine->GetRenderManager()->GetRootSignature()->GetComputeRootSignature()


#define FRAMERESOURCEIDNEX	gEngine->GetRenderManager().GetFrameResourceIndex()
#define INPUT				gEngine->GetInputManager()
#define DELTA_TIME			gEngine->GetTimer().GetTimeElapsed()

#define CONST_BUFFER(type,count) gEngine->GetRenderManager().GetConstantBuffer(type,count)
#define STRUCT_BUFFER(type,count) gEngine->GetRenderManager().GetStructuredBuffer(type,count)

