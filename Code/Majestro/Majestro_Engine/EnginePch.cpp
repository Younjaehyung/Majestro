#include "pch.h"
#include "EnginePch.h"
#include "Engine.h"

std::unique_ptr<Engine> gEngine = make_unique<Engine>();

// ANSI
void LogDebug(const std::string& msg) {
	std::string output = "[LOG] " + msg + "\n";
	OutputDebugStringA(output.c_str());
}

// Unicode
void LogDebugW(const std::wstring& msg) {
	std::wstring output = L"[LOG] " + msg + L"\n";
	OutputDebugStringW(output.c_str());
}

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
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	string r(len, '\0');
	::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

std::wstring utfs2ws(const std::string& utf8)
{
	if (utf8.empty()) return L"";
	const int wideLen = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
	std::wstring wide(wideLen, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), wide.data(), wideLen);
	return wide;
}

