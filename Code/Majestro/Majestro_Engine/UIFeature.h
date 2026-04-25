#pragma once
#include "World.h"
#include "UIRenderSystem.h"

class CameraComponent;

enum class WorldUIPassMode
{
    World,  // mIsScreenSpace=false 인 바만 그림 (기존 GameRenderPipeline 호출 경로용)
    HUD,    // mIsScreenSpace=true 인 바만 그림 (UIRenderSystem 이후 별도 호출용)
    All,    // 둘 다 (테스트/통합용)
};


class UIFeature
{
public:
    virtual ~UIFeature() = default;

    virtual void Initialize(World* world)
    {
        mWorld = world;
    }

    virtual void Update(float dt) {};

	virtual void WorldRender(CameraComponent* camera) {};

    virtual void SpriteRender(DirectX::SpriteBatch* spriteBatch) {};

    virtual void CustomSpriteRender(std::vector<UIInstanceData>& instances) {};

    // SpriteUpdate() 이후 SpriteBatch가 그린 결과 위에 커스텀 드로우 콜 가능.
    // instances: UIRenderSystem이 관리하는 인스턴스 벡터(정규 UI 데이터가 이미 들어있음).
    //            feature가 뒤에 append 후 UIInfo 버퍼에 재업로드하여 추가 인스턴스로 활용 가능.
    virtual void PostSpriteRender(std::vector<UIInstanceData>& instances) {};

protected:
    World* mWorld = nullptr;
};