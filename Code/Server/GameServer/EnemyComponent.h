#pragma once
#include "Component.h"
#include "Entity.h"

class EnemyComponent : public Component<EnemyComponent>
{
public:
	ComponentTypeID Target;
};