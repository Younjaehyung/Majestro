#pragma once
#include "Component.h"
#include "CameraView.h"

enum class MainMenuView : int
{
    Title    = 0,
    MainMenu = 1,
    Setting  = 2,
    Manual   = 3,
    RoomList = 4,
    Exit     = 5,
    Count
};


class MainMenuCameraComponent : public Component<MainMenuCameraComponent>
{
public:
    std::array<Cinematic::CameraView, (size_t)MainMenuView::Count> mViews{};
    bool mLoaded = false;

    MainMenuView mCurrent       = MainMenuView::Title;
    MainMenuView mTarget        = MainMenuView::Title;
    float        mBlendT        = 1.0f;   // 1=완료
    float        mBlendDuration = 0.65f;   // 초

    
    Vec3       mFromPos{};
    Quaternion mFromRot{};
    float      mFromFovDeg = 0.f;

    const Cinematic::CameraView& View(MainMenuView v) const { return mViews[(size_t)v]; }

    void RequestView(MainMenuView v, Vec3 curPos, Quaternion curRot, float curFovDeg)
    {
        if (!mLoaded) return;
        if ((int)v < 0 || (int)v >= (int)MainMenuView::Count) return;
        if (v == mTarget && mBlendT >= 1.f) return;
        mFromPos    = curPos;
        mFromRot    = curRot;
        mFromFovDeg = curFovDeg;
        mTarget     = v;
        mBlendT     = 0.f;
    }
};
