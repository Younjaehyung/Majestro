#pragma once
#include "Object.h"

struct ParticleEffectDesc
{
	wstring materialName = L"Particle";
	wstring computeMaterialName = L"ComputeParticle";
	uint32 maxParticle = 256;
	float createInterval = 0.02f;
	float minLifeTime = 0.8f;
	float maxLifeTime = 1.6f;
	float minSpeed = 25.f;
	float maxSpeed = 55.f;
	float startScale = 18.f;
	float endScale = 6.f;
	bool loop = true;
	bool autoDestroy = false;
};

class ParticleEffect : public Object
{
public:
	ParticleEffect();

	ParticleEffectDesc mDesc;
};
