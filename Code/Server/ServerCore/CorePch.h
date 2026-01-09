#pragma once


// Network
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include "CoreMacro.h"

#include <windows.h>
#include <iostream>
#include <debugapi.h>
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
using namespace std;
#include "Types.h"

#pragma comment(lib, "ws2_32.lib")
#include "../../Protocol/Packet.h"
#include "../../Protocol/PacketHelper.h"





