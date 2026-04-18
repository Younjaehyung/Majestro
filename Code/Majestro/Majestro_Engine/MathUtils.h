#pragma once

Vec4 HlslQuatMul(const Vec4& q1, const Vec4& q2);
Vec4 HlslQuatConj(const Vec4& q);

Vec4 HlslQuatSlerp(const Vec4& a, const Vec4& b, float t);
Vec4 LerpV4(const Vec4& a, const Vec4& b, float t);

Vec4 MulCompV4(const Vec4& a, const Vec4& b);

Vec4 DivCompV4(const Vec4& a, const Vec4& b);
Vec4 MaxV4(const Vec4& a, const Vec4& b);

float Saturate(float v);
