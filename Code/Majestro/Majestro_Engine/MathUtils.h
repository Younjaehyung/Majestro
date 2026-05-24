#pragma once

constexpr float kPI = 3.1415926535f;
constexpr float kDegToRad = 0.01745329251994329577f;

Vec4 HlslQuatMul(const Vec4& q1, const Vec4& q2);
Vec4 HlslQuatConj(const Vec4& q);
Vec4 QuatFromAxisAngle(const Vec3& axis, float rad);

Vec4 HlslQuatSlerp(const Vec4& a, const Vec4& b, float t);
Vec4 LerpV4(const Vec4& a, const Vec4& b, float t);

Vec4 MulCompV4(const Vec4& a, const Vec4& b);

Vec4 DivCompV4(const Vec4& a, const Vec4& b);
Vec4 MaxV4(const Vec4& a, const Vec4& b);

float Saturate(float v);
float Wrap180Degrees(float deg);
float SmoothStep01(float t);
float SmoothStep(float lo, float hi, float x);
float LerpAngleDegrees(float startYaw, float targetYaw, float alpha);

float EaseOutCubic(float t);
float EaseOutBack(float t);
float EaseInCubic(float t);
float DampedSine(float t, float freq, float damp);