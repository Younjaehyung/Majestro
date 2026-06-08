#pragma once

const json& RequireJson(const json& value, const char* key);
float GetOptionalFloat(const json& value, const char* key, float defaultValue = 0.f);
bool GetOptionalBool(const json& value, const char* key, bool defaultValue = 0.f);
std::string GetOptionalString(const json& value, const char* key, const std::string& defaultValue = "");
float ClampFloat(float value, float minValue, float maxValue);
Vec3 SafeNormalize(Vec3 value, const Vec3& fallback);
Vec3 ParseVec3ArrayOrObject(const json& value, float scale);
Vec3 ConvertUeVectorToDx(const Vec3& ue);
Matrix BuildPayloadWorldMatrix(const Vec3& position, Vec3 forward, Vec3 up);
float NormalizeDistanceForPath(float distance, float length, bool closedLoop);
bool ReadOptionalVec3(const std::wstring& path, const char* key, Vec3& out);

/////////////////////////////////////