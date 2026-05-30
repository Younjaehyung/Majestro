#pragma once
#include "Component.h"
#include "Entity.h"

class ParticleComponent : public Component<ParticleComponent>
{
public:
	enum class RenderMode : uint8
	{
		GeometryShaderBillboard,
		InstancedQuadBillboard
	};

public:
	wstring mEffectName = L"";
	wstring mMaterialName = L"Particle";
	wstring mComputeShaderName = L"ComputeParticle";
	bool mEffectDescApplied = false;
	RenderMode mRenderMode = RenderMode::GeometryShaderBillboard;

	uint32 mMaxParticle = 256;
	float mCreateInterval = 0.02f;
	float mAccTime = 0.f;

	float mMinLifeTime = 0.8f;
	float mMaxLifeTime = 1.6f;
	float mMinSpeed = 25.f;
	float mMaxSpeed = 55.f;
	float mStartScale = 18.f;
	float mEndScale = 6.f;

	bool mLoop = true;
	bool mAutoDestroy = false;
	bool mIsPlaying = true;
	bool mIsActive = true;
	bool mBurstTriggered = false;

	Entity mFollowTarget;
	Vec3 mFollowOffset = Vec3::Zero;
};

struct ParticleStateParams
{
	Vec3 worldPos{};
	float curTime = 0.f;
	Vec3 worldDir{};
	float lifeTime = 0.f;
	int alive = 0;
	float padding[3]{};
};

struct ParticleSharedParams
{
	Matrix MatWorld = Matrix::Identity;
	int TextureIndex = -1;
	int maxCount = 0;
	int addCount = 0;
	float deltaTime = 0.f;
	float accTime = 0.f;
	float minLifeTime = 0.f;
	float maxLifeTime = 0.f;
	float minSpeed = 0.f;
	float maxSpeed = 0.f;
	float StartScale = 1.f;
	float EndScale = 1.f;
};

struct ParticleComputeSharedParams
{
    int addCount = 0;
    int padding[3]{};
};

