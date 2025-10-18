#pragma once
#include "Component.h"
#include "TransformComponent.h"

enum PlayMode 
{
	MAIN_CAMERA,
	ONE_FPS,
	THREE_FPS,
	THREE_RPG,
};

class PlayerComponent : public Component<PlayerComponent>
{
public:
	PlayerComponent() {}
	PlayerComponent(TransformComponent transform): mTransformComponent(transform) {}
	PlayerComponent(TransformComponent transform, PlayMode mode) : mTransformComponent(transform), mPlayMode(mode) {}
public:
	TransformComponent mTransformComponent;
	PlayMode mPlayMode;
	float mHight = 1; //모델 심장부
	float mCameraLenth = 5; //캐릭터 카메라 거리(차후 충돌처리 할일 생기면 스프링 암으로 변경)
};