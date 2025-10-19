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
	PlayerComponent() : mTransformComponent(), mPlayMode(MAIN_CAMERA) {}
	PlayerComponent(TransformComponent transform): mTransformComponent(transform), mPlayMode() {}
	PlayerComponent(TransformComponent transform, PlayMode mode) : mTransformComponent(transform), mPlayMode(mode) {}
public:
	TransformComponent mTransformComponent;
	PlayMode mPlayMode;
	float mHight = 1; //�� �����
	float mCameraLenth = 5; //ĳ���� ī�޶� �Ÿ�(���� �浹ó�� ���� ����� ������ ������ ����)
};