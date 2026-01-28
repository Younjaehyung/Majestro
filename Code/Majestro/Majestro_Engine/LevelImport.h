#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"
using json = nlohmann::json;


struct Basis
{
    Vec3 forward; // Z축으로 쓸지, forward로 쓸지는 엔진 규약에 맞춰 처리
    Vec3 right;
    Vec3 up;
};

struct TransformData
{
    Vec3 position;
    Vec3 scale;
    Basis basis;   // 회전은 basis로 안전하게 전달(UE Euler 함정 회피)
};

struct MeshInstance
{
    std::string actorName;
    std::string actorPath;

    std::string componentName;
    std::string staticMeshAsset; // UE 에셋 경로(디버깅/매핑용)
    std::string fbx;             // "Meshes/SM_xxx.fbx"

    TransformData world;         // dx 기준(transform.dx)
};

struct LevelImportData
{
    std::string levelName;
    std::string actualExportRoot; // JSON에 들어있던 값(참고용)
    std::vector<MeshInstance> instances;
};

// 안전 파싱 유틸: 키 없으면 예외 (너 성격상 "틀리면 바로 잡자"라서 강경하게 감)
static const json& Require(const json& j, const char* key)
{
    auto it = j.find(key);
    if (it == j.end())
        throw std::runtime_error(std::string("JSON missing key: ") + key);
    return *it;
}

static float GetFloat(const json& j, const char* key)
{
    return Require(j, key).get<float>();
}

static std::string GetString(const json& j, const char* key)
{
    return Require(j, key).get<std::string>();
}

static Vec3 ParseVec3(const json& j)
{
    Vec3 v;
    v.x = GetFloat(j, "x");
    v.y = GetFloat(j, "y");
    v.z = GetFloat(j, "z");
    return v;
}

static Basis ParseBasis(const json& jBasis)
{
    Basis b{};
    b.forward = ParseVec3(Require(jBasis, "forward"));
    b.right = ParseVec3(Require(jBasis, "right"));
    b.up = ParseVec3(Require(jBasis, "up"));
    return b;
}

static TransformData ParseDxTransform(const json& jDx)
{
    TransformData t{};
    t.position = ParseVec3(Require(jDx, "location"));
    t.scale = ParseVec3(Require(jDx, "scale"));
    t.basis = ParseBasis(Require(jDx, "basis"));
    return t;
}


static inline DirectX::XMMATRIX BuildWorldMatrix_RowMajor(const TransformData& t)
{
    using namespace DirectX;

    const XMVECTOR r = XMVectorSet(t.basis.right.x, t.basis.right.y, t.basis.right.z, 0.f);
    const XMVECTOR u = XMVectorSet(t.basis.up.x, t.basis.up.y, t.basis.up.z, 0.f);
    const XMVECTOR f = XMVectorSet(t.basis.forward.x, t.basis.forward.y, t.basis.forward.z, 0.f);

    // 회전 행렬(축 벡터가 정규화되어 있다는 가정)
    // row-major: 각 행이 basis 축
    XMMATRIX rot =
    {
        XMVectorSetW(r, 0.f),
        XMVectorSetW(u, 0.f),
        XMVectorSetW(f, 0.f),
        XMVectorSet(0.f, 0.f, 0.f, 1.f)
    };

    const XMMATRIX scl = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z);
    const XMMATRIX trn = XMMatrixTranslation(t.position.x, t.position.y, t.position.z);

    // 보통: S * R * T (너가 엔진에서 쓰는 순서로 맞춰라)
    return scl * rot * trn;
}
