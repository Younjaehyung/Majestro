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
#include "GameTimer.h"

#include "SimpleMath.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace DirectX::SimpleMath;

// RecastNavigation
#include <RecastNavigation/Detour/DetourNavMesh.h>
#include <RecastNavigation/Detour/DetourNavMeshQuery.h>
#include <RecastNavigation/Detour/DetourNavMeshBuilder.h>
#include <RecastNavigation/Detour/DetourAlloc.h>

#ifdef _DEBUG
#pragma comment(lib, "RecastNavigation/Detour-d.lib")
#pragma comment(lib, "RecastNavigation/DebugUtils-d.lib")
#else
#pragma comment(lib, "RecastNavigation/Detour.lib")
#pragma comment(lib, "RecastNavigation/DebugUtils.lib")
#endif


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


wstring s2ws(const string& s);
string ws2s(const wstring& s);

using Vec = XMVECTOR;
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;
using Quternion = DirectX::SimpleMath::Quaternion;

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


extern unique_ptr<class GameCore> gGameCore;
extern unique_ptr<class ServerCore> gServerCore;

#define RESOURCEMANAGER	gGameCore->GetResourceManager()
#define SCENEMANAGER	gGameCore->GetSceneManager()



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