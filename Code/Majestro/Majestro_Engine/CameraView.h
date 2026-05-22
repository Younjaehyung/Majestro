#pragma once


struct CameraView
{
    Vec3       position{};
    Quaternion rotation{};
    float      fovDeg = 0.f;
};



void ConvertMainMenuSample(const json& s, CameraView& v);
bool IsSameMainMenuStop(const CameraView& a, const CameraView& b);