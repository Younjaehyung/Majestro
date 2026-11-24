#include "pch.h"
#include "GameCore.h"
#include "ServerCore.h"

std::unique_ptr<GameCore> gGameCore = make_unique<GameCore>();
std::unique_ptr<ServerCore> gServerCore = make_unique<ServerCore>();

wstring s2ws(const string& s)
{
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	::MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	wstring ret(buf);
	delete[] buf;
	return ret;
}

string ws2s(const wstring& s)
{
	// 길이에 널 문자를 넣지 않는다 (넣으면 문자열 중간에 '\0' 이 박혀 이어 붙이기·로그가 잘린다)
	if (s.empty())
		return string();
	const int32 slength = static_cast<int32>(s.length());
	const int32 len = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	string r(len, '\0');
	::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

