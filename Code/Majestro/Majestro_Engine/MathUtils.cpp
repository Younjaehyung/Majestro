#include "pch.h"
#include "MathUtils.h"


Vec4 HlslQuatMul(const Vec4& q1, const Vec4& q2)
{
	Vec4 r;
	r.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	r.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	r.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
	r.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	const float n2 = r.x * r.x + r.y * r.y + r.z * r.z + r.w * r.w;
	if (n2 > 1e-12f)
	{
		const float inv = 1.f / std::sqrt(n2);
		r.x *= inv; r.y *= inv; r.z *= inv; r.w *= inv;
	}
	return r;
}

Vec4 HlslQuatConj(const Vec4& q)
{
	return Vec4(-q.x, -q.y, -q.z, q.w);
}

// HLSL QuaternionSlerp
Vec4 HlslQuatSlerp(const Vec4& a, const Vec4& b, float t)
{
	return Vec4(XMQuaternionSlerp(a, b, t));
}


Vec4 LerpV4(const Vec4& a, const Vec4& b, float t)
{
	return Vec4(XMVectorLerp(a, b, t));
}


Vec4 MulCompV4(const Vec4& a, const Vec4& b)
{
	return Vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

Vec4 DivCompV4(const Vec4& a, const Vec4& b)
{
	return Vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

Vec4 MaxV4(const Vec4& a, const Vec4& b)
{
	return Vec4(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w));
}


float Saturate(float v)
{
	return std::clamp(v, 0.f, 1.f);
}
