#include "pch.h"
#include "JsonUtils.h"

const json& RequireJson(const json& value, const char* key)
{
	auto it = value.find(key);
	if (it == value.end())
		throw std::runtime_error(std::string("JSON missing key: ") + key);
	return *it;
}

float GetFloat(const json& value, const char* key)
{
	return RequireJson(value, key).get<float>();
}

std::string GetString(const json& value, const char* key)
{
	return RequireJson(value, key).get<std::string>();
}

float GetOptionalFloat(const json& value, const char* key, float defaultValue)
{
	auto it = value.find(key);
	if (it == value.end() || it->is_null())
		return defaultValue;
	return it->get<float>();
}

bool GetOptionalBool(const json& value, const char* key, bool defaultValue)
{
	auto it = value.find(key);
	if (it == value.end() || it->is_null())
		return defaultValue;
	return it->get<bool>();
}

std::string GetOptionalString(const json& value, const char* key, const std::string& defaultValue)
{
	auto it = value.find(key);
	if (it == value.end() || it->is_null())
		return defaultValue;
	return it->get<std::string>();
}

float ClampFloat(float value, float minValue, float maxValue)
{
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;
	return value;
}

Vec3 SafeNormalize(Vec3 value, const Vec3& fallback)
{
	if (value.LengthSquared() <= 0.000001f)
		return fallback;

	value.Normalize();
	return value;
}

Vec3 ParseVec3ArrayOrObject(const json& value, float scale)
{
	if (value.is_array() && value.size() >= 3)
		return Vec3(value[0].get<float>() * scale, value[1].get<float>() * scale, value[2].get<float>() * scale);

	if (value.is_object())
		return Vec3(
			RequireJson(value, "x").get<float>() * scale,
			RequireJson(value, "y").get<float>() * scale,
			RequireJson(value, "z").get<float>() * scale);

	throw std::runtime_error("Payload path JSON vec3 must be [x,y,z] array or {x,y,z} object");
}

Vec3 ConvertUeVectorToDx(const Vec3& ue)
{
	// 클라이언트 엔진 좌표계(X 우측, Y 상단, Z 전방)로 변환
	return Vec3(ue.y, ue.z, ue.x);
}

Matrix BuildPayloadWorldMatrix(const Vec3& position, Vec3 forward, Vec3 up)
{
	forward = SafeNormalize(forward, Vec3::Backward);
	up = up - forward * up.Dot(forward);
	up = SafeNormalize(up, Vec3::Up);

	Vec3 right = up.Cross(forward);
	right = SafeNormalize(right, Vec3::Right);
	forward = right.Cross(up);
	forward = SafeNormalize(forward, Vec3::Backward);

	return Matrix(
		right.x, right.y, right.z, 0.f,
		up.x, up.y, up.z, 0.f,
		-forward.x, -forward.y, -forward.z, 0.f,
		position.x, position.y, position.z, 1.f);
}

float NormalizeDistanceForPath(float distance, float length, bool closedLoop)
{
	if (length <= 0.f)
		return 0.f;

	if (closedLoop)
	{
		distance = std::fmod(distance, length);
		if (distance < 0.f)
			distance += length;
		return distance;
	}

	return ClampFloat(distance, 0.f, length);
}



///////////////////////////////////

