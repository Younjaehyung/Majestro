#pragma once


namespace Cinematic
{
    struct CameraView
    {
        Vec3       position{};
        Quaternion rotation{};
        float      fovDeg = 0.f;
    };


    // 시간축을 가진 시네마틱 카메라 키프레임.
    struct CameraKeyframe
    {
        CameraView view{};
        float      seconds = 0.f;
    };



    void ConvertMainMenuSample(const json& s, CameraView& v);
    bool IsSameMainMenuStop(const CameraView& a, const CameraView& b);
    CameraView SampleCameraSequence(const std::vector<CameraKeyframe>& keys, float t);
}