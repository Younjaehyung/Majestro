#pragma once
#include "Component.h"

class StructuredBuffer;
class Material;
class Mesh;


class ParticleComponent : public Component<ParticleComponent>
{
public:

	shared_ptr<StructuredBuffer>	_particleBuffer;
	shared_ptr<StructuredBuffer>	_computeSharedBuffer;
	uint32							_maxParticle = 1000;

	shared_ptr<Material>		_computeMaterial;
	shared_ptr<Material>		_material;
	shared_ptr<Mesh>			_mesh;

	float				_createInterval = 0.005f;
	float				_accTime = 0.f;

	float				_minLifeTime = 0.5f;
	float				_maxLifeTime = 1.f;
	float				_minSpeed = 100;
	float				_maxSpeed = 50;
	float				_startScale = 10.f;
	float				_endScale = 5.f;
};


struct ParticleParms
{
    int Index;
    Matrix MatWorld;

    int maxCount;
    int addCount;
    int frameNumber;
    float deltaTime;
    float accTime;
    float minLifeTime;
    float maxLifeTime;
    float minSpeed;
    float maxSpeed;


    Vec3 worldPos;
    float curTime; //경과시간
    Vec3 worldDir;
    float lifeTime; //유지시간
    int alive; //랜더링유무용

    float EndScale;
    float StartScale;
};