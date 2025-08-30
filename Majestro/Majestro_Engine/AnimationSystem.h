#pragma once
#include "World.h"
#include "System.h"


class AnimationSystem : public System
{
public:
	AnimationSystem(World* world);
	void Initialize();

	void Update(float);

private:	// COMPUTE 애니메이션 시스템

	void ClearVector();

	void AnimationBlend(float);
	void AnimationUpdate(float);
	void AnimationCompute();
private:	// CPU 애니메이션 시스템


private:
	vector<Matrix>	mAnimationVector;
};
