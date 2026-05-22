#pragma once
#include "json.hpp"
using json = nlohmann::json;
#include "JsonUtils.h"   // RequireJson / GetFloat / GetString / ParseVec3ArrayOrObject / ConvertUeVectorToDx


struct Basis
{
    Vec3 forward; 
    Vec3 right;
    Vec3 up;
};

struct TransformData
{
    Vec3 position;
    Vec3 scale;
	Vec3 rotation; // UE Euler 각도(디버깅/매핑용)
    Basis basis;   
};

struct MeshInstance
{
    std::string actorName;
    std::string actorPath;

    std::string componentName;
    std::string staticMeshAsset; // UE 에셋 경로(디버깅/매핑용)
    std::string fbx;             // "Meshes/SM_xxx.fbx"

	TransformData ue;            // ue 기준(transform.ue)
    TransformData world;         // dx 기준(transform.dx)
    Matrix worldMtx; //  DX12에 바로 넣을 월드행렬 캐시
};

struct LevelImportData
{
    std::string levelName;
    std::string actualExportRoot; // JSON에 들어있던 값(참고용)
    std::vector<MeshInstance> instances;
};


// JSON 파싱 공통 헬퍼는 JsonUtils.h 로 통합됨
// (RequireJson / GetFloat / GetString / ParseVec3ArrayOrObject / ConvertUeVectorToDx).
// 아래는 LevelImport 도메인(Basis/TransformData) 전용 파서다.

static Vec3 ParseRot3(const json& j)
{
    Vec3 v;
    v.x = GetFloat(j, "pitch");
    v.y = GetFloat(j, "yaw");
    v.z = GetFloat(j, "roll");
    return v;
}

static Basis ParseBasis(const json& jBasis)
{
    Basis b{};
    b.forward = ParseVec3ArrayOrObject(RequireJson(jBasis, "forward"), 1.f);
    b.right   = ParseVec3ArrayOrObject(RequireJson(jBasis, "right"), 1.f);
    b.up      = ParseVec3ArrayOrObject(RequireJson(jBasis, "up"), 1.f);
    return b;
}

static TransformData ParseDxTransform(const json& jDx)
{
    TransformData t{};
    t.position = ParseVec3ArrayOrObject(RequireJson(jDx, "location"), 1.f);
    t.scale    = ParseVec3ArrayOrObject(RequireJson(jDx, "scale"), 1.f);
    t.basis    = ParseBasis(RequireJson(jDx, "basis"));
    return t;
}

static TransformData ParseUETransform(const json& jUe, float positionUnitScale)
{
    TransformData t{};
    t.position = ParseVec3ArrayOrObject(RequireJson(jUe, "location_cm"), 1.f);
    t.scale    = ParseVec3ArrayOrObject(RequireJson(jUe, "scale"), 1.f);
    t.rotation = ParseRot3(RequireJson(jUe, "rotation_deg"));
    t.basis    = ParseBasis(RequireJson(jUe, "basis"));
    return t;
}

static inline Basis ConvertUeToDx(const Basis& b)
{
    Basis out{};
    out.right   = ConvertUeVectorToDx(b.right);
    out.up      = ConvertUeVectorToDx(b.up);
    out.forward = ConvertUeVectorToDx(b.forward);
    return out;
}

static inline Vec3 NormalizeVec3(const Vec3& v)
{

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
  
    b.forward = NormalizeVec3(b.forward);


    b.up = SubVec3(b.up, MulVec3(b.forward, DotVec3(b.up, b.forward)));
    b.up = NormalizeVec3(b.up);

  
    b.right = CrossVec3(b.up, b.forward);
    b.right = NormalizeVec3(b.right);

  
    b.forward = CrossVec3(b.right, b.up);
    b.forward = NormalizeVec3(b.forward);
}

static inline DirectX::XMMATRIX MakeRotation_RowBasis(const Basis& basis)
{
    const Vec3 r = basis.right;
    const Vec3 u = basis.up;
    const Vec3 f = basis.forward;

    return DirectX::XMMATRIX(
        r.x, r.y, r.z, 0.f,
        u.x, u.y, u.z, 0.f,
        f.x, f.y, f.z, 0.f,
        0.f, 0.f, 0.f, 1.f
    );
}

static inline Matrix BuildWorldMatrix_RowMajor(const TransformData& dx, bool fromUe = false)
{
    Vec3 r = dx.basis.right;   
    Vec3 u = dx.basis.up;      
    Vec3 f = -dx.basis.forward; 

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


 
    return worldCpu;


}


static DirectX::XMFLOAT3 ConvertPosition(const Vec3& uePos)
{


    return DirectX::XMFLOAT3(
        uePos.y,  // Unreal Y (Right)  DX X (Right)
        uePos.z,  // Unreal Z (Up)     DX Y (Up)
        uePos.x   // Unreal X (Fwd)    DX Z (Forward)
    );
   
}

static DirectX::XMVECTOR ConvertRotation(const Vec3& ueRot)
{

    float pitchRad = DirectX::XMConvertToRadians(ueRot.x);
    float yawRad = DirectX::XMConvertToRadians(ueRot.y);
    float rollRad = DirectX::XMConvertToRadians(ueRot.z);

   return Quaternion::CreateFromYawPitchRoll(yawRad, pitchRad, rollRad);


}

static DirectX::XMFLOAT3 ConvertScale(const Vec3& ueScale)
{
    // 스케일도 축 재배열
    return DirectX::XMFLOAT3(
        ueScale.y,  
        ueScale.z,  
        ueScale.x   
    );
}

static Matrix ConvertTransform(
    const TransformData& ueTransform)
{
    DirectX::XMFLOAT3 pos = ConvertPosition(ueTransform.position);
    DirectX::XMVECTOR rot = ConvertRotation(ueTransform.rotation);
    DirectX::XMFLOAT3 scale = ConvertScale(ueTransform.scale);

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(rot);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

    return S * R * T;
}