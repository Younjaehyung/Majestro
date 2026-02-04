#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

//#ifdef _DEBUG
//#pragma comment(lib, "Debug\\ServerCore.lib")
//#else
//#pragma comment(lib, "Release\\ServerCore.lib")
//#endif

//#include "CorePch.h"


// Network
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib, "ws2_32.lib")

#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include "SimpleMath.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace DirectX::PackedVector;




using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;



using Vec = XMVECTOR;
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;


#include "../../Protocol/Packet.h"
#include "PacketHelper.h"
using namespace std;

// ANSI
static void LogDebug(const std::string& msg) {
	std::string output = "[LOG] " + msg + "\n";
	OutputDebugStringA(output.c_str());
}

// Unicode
static void LogDebugW(const std::wstring& msg) {
	std::wstring output = L"[LOG] " + msg + L"\n";
	OutputDebugStringW(output.c_str());
}