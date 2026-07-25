#pragma once

class World;

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


    void ApplyCameraSequence(World* world, const std::vector<CameraKeyframe>& keys, float elapsed);     // 시퀀스 재생


    bool IsAnyCinematicPlaying(World* world);    // 어느 시네마틱이라도 재생 중인지


    void RequestPlazaLevelEnter(World* world, SceneId target);      // 레벨 진입 요청
}