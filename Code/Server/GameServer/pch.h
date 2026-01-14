#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

//std::byte 사용안함 설정
#define _HAS_STD_BYTE 0

#ifdef _DEBUG
#pragma comment(lib, "Debug\\ServerCore.lib")
#else
#pragma comment(lib, "Release\\ServerCore.lib")
#endif

#include "CorePch.h"

#include "SimpleMath.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

//
//#include <DirectXTex/DirectXTex.h>
//#include <DirectXTex/DirectXTex.inl>
//
//// DirectXTex
//#ifdef _DEBUG
//#pragma comment(lib, "DirectXTex\\DirectXTex_Debug.lib")
//#else
//#pragma comment(lib, "DirectXTex\\DirectXTex.lib")
//#endif


using Vec = XMVECTOR;
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;