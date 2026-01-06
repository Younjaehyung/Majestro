#pragma once

#include "Types.h"
#include "CoreMacro.h"


// Network
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

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

#pragma comment(lib, "ws2_32.lib")

using namespace std;

