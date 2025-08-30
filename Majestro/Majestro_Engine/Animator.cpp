#include "pch.h"
#include "Animator.h"
#include "FBXData.h"

Animator::Animator() : Object(OBJECT_TYPE::ANIMATION)
{
}

Animator::~Animator()
{
}

vector<Animator> Animator::CreateAnimations(FileLoader& loader)
{
}
