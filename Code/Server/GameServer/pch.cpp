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
	int32 len;
	int32 slength = static_cast<int32>(s.length()) + 1;
	len = ::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
	string r(len, '\0');
	::WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0);
	return r;
}

std::mt19937& RandomEngine()
{
	static thread_local std::mt19937 engine{ std::random_device{}() };
	return engine;
}

Vec3 SampleDiskXZ(float radius)
{
	if (radius <= 0.0f) return Vec3(0.0f, 0.0f, 0.0f);

	std::uniform_real_distribution<float> angleDist(0.0f, 6.28318530718f);
	std::uniform_real_distribution<float> rDist(0.0f, 1.0f);
	const float angle = angleDist(RandomEngine());
	const float r = radius * std::sqrt(rDist(RandomEngine()));
	return Vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
}