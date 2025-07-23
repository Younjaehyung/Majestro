#pragma once
#include "Component.h"
#include "Shader.h"

class Mesh;
class Material;

class RenderComponent
{
public:


	Mesh* mMesh;
	Material* mMaterial;

};

