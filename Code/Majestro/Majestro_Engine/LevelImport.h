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
    Matrix worldMtx; // [수정] DX12에 바로 넣을 월드행렬 캐시
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

static inline Vec3 ConvertUeToDx(const Vec3& v)
{
    // UE: X forward, Y right, Z up
    // DX: X right, Y up, Z forward
    // => (x, y, z) -> (y, z, x)
    return Vec3{ v.y, v.z, v.x };
}

static inline Basis ConvertUeToDx(const Basis& b)
{
    Basis out{};
    out.right = ConvertUeToDx(b.right);
    out.up = ConvertUeToDx(b.up);
    out.forward = ConvertUeToDx(b.forward);
    return out;
}

static inline Vec3 NormalizeVec3(const Vec3& v)
{
    // [수정] basis 안정화를 위해 정규화 함수 추가
    DirectX::XMVECTOR vv = DirectX::XMVectorSet(v.x, v.y, v.z, 0.f);
    vv = DirectX::XMVector3Normalize(vv);
    Vec3 out{};
    out.x = DirectX::XMVectorGetX(vv);
    out.y = DirectX::XMVectorGetY(vv);
    out.z = DirectX::XMVectorGetZ(vv);
    return out;
}

static inline Vec3 CrossVec3(const Vec3& a, const Vec3& b)
{
    DirectX::XMVECTOR va = DirectX::XMVectorSet(a.x, a.y, a.z, 0.f);
    DirectX::XMVECTOR vb = DirectX::XMVectorSet(b.x, b.y, b.z, 0.f);
    DirectX::XMVECTOR vc = DirectX::XMVector3Cross(va, vb);
    Vec3 out{};
    out.x = DirectX::XMVectorGetX(vc);
    out.y = DirectX::XMVectorGetY(vc);
    out.z = DirectX::XMVectorGetZ(vc);
    return out;
}

static inline float DotVec3(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 SubVec3(const Vec3& a, const Vec3& b)
{
    return Vec3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vec3 MulVec3(const Vec3& a, float s)
{
    return Vec3{ a.x * s, a.y * s, a.z * s };
}

static inline void OrthonormalizeBasis(Basis& b)
{
    // [수정] basis가 약간 틀어져도 월드행렬이 안정적으로 나오게 직교정규화
    // 엔진 좌표계: X=right, Y=up, Z=forward(+Z) (우수계로 가정: right = up x forward)

    b.forward = NormalizeVec3(b.forward);

    // up에서 forward 성분 제거(Gram-Schmidt)
    b.up = SubVec3(b.up, MulVec3(b.forward, DotVec3(b.up, b.forward)));
    b.up = NormalizeVec3(b.up);

    // right = up x forward  (X=right, Y=up, Z=forward(+Z) 일 때 가장 자연스러운 정의)
    b.right = CrossVec3(b.up, b.forward);
    b.right = NormalizeVec3(b.right);

    // forward도 다시 보정 (forward = right x up)
    b.forward = CrossVec3(b.right, b.up);
    b.forward = NormalizeVec3(b.forward);
}

// [수정] "row-vector(v*M) + S*R*T" 기준으로 회전행렬 구성
// row0 = right, row1 = up, row2 = forward
static inline DirectX::XMMATRIX MakeRotation_RowBasis(const Basis& basis)
{
    const Vec3 r = basis.right;
    const Vec3 u = basis.up;
    const Vec3 f = basis.forward;

    // [수정] 기존 코드는 (r.x, u.x, f.x) 식으로 들어가 사실상 column-basis 형태였음.
    // 여기서는 row-basis로 확정한다.
    return DirectX::XMMATRIX(
        r.x, r.y, r.z, 0.f,
        u.x, u.y, u.z, 0.f,
        f.x, f.y, f.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

static inline Matrix BuildWorldMatrix_RowMajor(const TransformData& dx, bool fromUe = false)
{
    const Vec3 r = dx.basis.right;    // 엔진 기준 right(+X)
    const Vec3 u = dx.basis.up;       // 엔진 기준 up(+Y)
    const Vec3 f = dx.basis.forward;  // 엔진 기준 forward(+Z)

    const float sx = dx.scale.x;
    const float sy = dx.scale.y;
    const float sz = dx.scale.z;

    const float px = dx.position.x;
    const float py = dx.position.y;
    const float pz = dx.position.z;


    const Matrix worldCpu =
    {
	   r.x* sx, r.y* sx, r.z* sx, 0.f,
	   u.x* sy, u.y* sy, u.z* sy, 0.f,
	   f.x* sz, f.y* sz, f.z* sz, 0.f,
	   px,      py,      pz,      1.f
    };

    const Matrix RotationOnly =
	{
		r.x, r.y, r.z, 0.f,
		u.x, u.y, u.z, 0.f,
		f.x, f.y, f.z, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

    const Matrix Identity =
    {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f };

    Matrix matScale = Matrix::CreateScale(sx,sy,sz);
    Matrix matTranslation = Matrix::CreateTranslation(px,py,pz);

	// X축 회전행렬 생성 예시
	Matrix RotateX = Matrix::CreateRotationX(DirectX::XMConvertToRadians(90.0f));

    return worldCpu;


}
