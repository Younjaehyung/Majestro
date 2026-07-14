#pragma once

class World;
class CameraComponent;
class DecalComponent;

class DecalPass
{
public:
    DecalPass()  = default;
    ~DecalPass() = default;

    void Initialize(World* world);
    void Execute(CameraComponent* camera);   // camera: 프러스텀 컬링용(nullptr이면 컬링 생략)

    bool IsEnabled() const { return mEnabled; }
    void SetEnabled(bool enabled) { mEnabled = enabled; }

private:
    World* mWorld   = nullptr;
    bool   mEnabled = true;

    struct Draw { DecalComponent* d; float fadeAlpha; };

    std::vector<Draw> mDraws;
};
